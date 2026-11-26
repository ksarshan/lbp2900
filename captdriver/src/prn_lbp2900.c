/*
 * Copyright (C) 2013 Alexey Galakhov <agalakhov@gmail.com>
 * Copyright (C) 2016 Alexei Gordeev <KP1533TM2@gmail.com>
 *
 * Licensed under the GNU General Public License Version 3
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * prn_lbp2900.c - CAPT job/page sequencing for the LBP2900 family.
 *
 * Also registers the closely related LBP3000, LBP3010/3018/3050,
 * LBP3100/3108/3150 and LBP6000/6018, which share the page logic and differ
 * only in job start-up, LED control and status strategy.
 *
 * One page goes through:
 *   page_prologue  wake the engine if needed, wait for buffer space, then
 *                  send the page and Hi-SCoA parameters;
 *   send_band      (generic-ops.c) the compressed bands;
 *   page_epilogue  end the data, wait until the page is received, "fire" it,
 *                  and wait until it is ejected (or report out of paper).
 *
 * The magic byte arrays below were taken from USB captures of Canon's
 * Windows driver; their individual fields are mostly undocumented.
 */

#include "std.h"
#include "word.h"
#include "capt-command.h"
#include "capt-status.h"
#include "generic-ops.h"
#include "hiscoa-common.h"
#include "hiscoa-compress.h"
#include "paper.h"
#include "printer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Job ID assigned by the printer in reply to CAPT_JOB_BEGIN. */
static uint16_t job;

/* Safety nets: with capt_get_xstatus_only the page counters converge in well
 * under a second, so these bounds are almost never hit — they exist purely so a
 * freak page whose status never settles can't hang the whole queue forever.
 * At CAPT_POLL_US (100 ms) per poll: 200 -> ~20 s, 250 -> ~25 s. */
enum {
	CAPT_WAIT_POLLS_PAGE = 200,
	CAPT_WAIT_POLLS_JOB  = 250,
};

/* CAPT_GPIO payloads: LED steady (init) and LED blinking (blink). */
struct printer_gpio_s {
	const uint8_t (*init);
	const uint8_t (*blink);
};

/*
 * Model ops: the generic printer_ops_s plus the status strategy. The generic
 * ops are the first member, so container_of() recovers this struct from the
 * printer_ops_s pointer that rastertocapt.c passes around.
 */
struct lbp2900_ops_s {
	struct printer_ops_s ops;
	struct printer_gpio_s gpio;

	const struct capt_status_s * (*get_status) (void);
	void (*wait_ready) (void);
};

static const uint8_t magicbuf_0[] = {
	0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t magicbuf_2[] = {
	0xEE, 0xDB, 0xEA, 0xAD, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t lbp2900_gpio_blink[] = {
	0x00, 0x00, 0x01, 0x02, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x00,
};

static const uint8_t lbp2900_gpio_init[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

static const uint8_t lbp3000_job_init[] = {
	0x00, 0x00,
};

static const uint8_t lbp3010_gpio_blink[] = {
	/* led */ 0x31, 0x00, 0x00, /* S6 */ 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, /* S7 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t lbp3010_gpio_init[] = {
	/* led */ 0x13, 0x00, 0x00, /* S6 */ 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, /* S7 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t lbp6000_job_init[] = {
	0x01, 0x00,
};

/* Query status using this model's strategy (basic+conditional or extended). */
static const struct capt_status_s *lbp2900_get_status(const struct printer_ops_s *ops)
{
	const struct lbp2900_ops_s *lops = container_of(ops, struct lbp2900_ops_s, ops);
	return lops->get_status();
}

/* Wait until not busy, using this model's status strategy. */
static void lbp2900_wait_ready(const struct printer_ops_s *ops)
{
	const struct lbp2900_ops_s *lops = container_of(ops, struct lbp2900_ops_s, ops);
	lops->wait_ready();
}

/*
 * Send CAPT_JOB_SETUP. @fg selects the stage: 1 = job start, 2 = page data
 * sent, 6 = page fired, 4 = job finished. @page is the page counter the stage
 * refers to. Host, user and document names are left empty.
 */
static void send_job_start(uint8_t fg, uint16_t page)
{
	uint8_t ml = 0x00; /* host name length */
	uint8_t ul = 0x00; /* user name length */
	uint8_t nl = 0x00; /* document name length */
	time_t rawtime = time(NULL);
	const struct tm *tm = localtime(&rawtime);
	uint8_t buf[32 + 40 + ml + ul + nl];
	uint8_t head[32] = {
		0x00, 0x00, 0x00, 0x00, LO(page), HI(page), 0x00, 0x00,
		ml, 0x00, ul, 0x00, nl, 0x00, 0x00, 0x00,
		fg, 0x01, LO(job), HI(job),
		/*-60 */ 0xC4, 0xFF,
		/*-120*/ 0x88, 0xFF,
		LO(tm->tm_year), HI(tm->tm_year), (uint8_t) tm->tm_mon, (uint8_t) tm->tm_mday,
		(uint8_t) tm->tm_hour, (uint8_t) tm->tm_min, (uint8_t) tm->tm_sec,
		0x01,
	};
	memcpy(buf, head, sizeof(head));
	memset(buf + 32, 0, 40 + ml + ul + nl);
	capt_sendrecv(CAPT_JOB_SETUP, buf, sizeof(buf), NULL, 0);
}

/*
 * Common start of every job prologue: identify, reset the cached status,
 * open a job and remember the job ID the printer assigns.
 */
static void begin_job(struct printer_state_s *state)
{
	uint8_t buf[8] = { 0 };
	/* Capacity of buf: capt_sendrecv() copies at most this many bytes. */
	size_t size = sizeof(buf);

	capt_sendrecv(CAPT_IDENT, NULL, 0, NULL, 0);
	sleep(1);
	capt_init_status();
	lbp2900_get_status(state->ops);

	capt_sendrecv(CAPT_START_0, NULL, 0, NULL, 0);
	capt_sendrecv(CAPT_JOB_BEGIN, magicbuf_0, ARRAY_SIZE(magicbuf_0), buf, &size);
	job = WORD(buf[2], buf[3]);
}

static void lbp2900_job_prologue(struct printer_state_s *state)
{
	/* Clear any stale printer-state-reason at the start of a job so a
	 * previous "out of paper" doesn't linger in the macOS queue window. */
	fprintf(stderr, "STATE: -media-empty\n");

	begin_job(state);

	capt_sendrecv(CAPT_GPIO, lbp3010_gpio_init, ARRAY_SIZE(lbp3010_gpio_init), NULL, 0);
	lbp2900_wait_ready(state->ops);

	send_job_start(1, 0);
	lbp2900_wait_ready(state->ops);
}

static void lbp3000_job_prologue(struct printer_state_s *state)
{
	begin_job(state);

	/* LBP-3000 prints the very first printjob perfectly
	 * and then proceeds to hang at this (commented out)
	 * spot. That's the difference, or so it seems. */
/*	lbp2900_wait_ready(state->ops);	*/
	send_job_start(1, 0);

	/* There's also that command, that apparently does something, and does something,
	 * but it's there in the Wireshark logs. Response data == command data. */
	capt_sendrecv(CAPT_LBP3000_SETUP_0, lbp3000_job_init, ARRAY_SIZE(lbp3000_job_init), NULL, 0);

	lbp2900_wait_ready(state->ops);
}

static void lbp3010_job_prologue(struct printer_state_s *state)
{
	begin_job(state);

	capt_sendrecv(CAPT_GPIO, lbp3010_gpio_init, ARRAY_SIZE(lbp3010_gpio_init), NULL, 0);
	lbp2900_wait_ready(state->ops);

	send_job_start(1, 0);
	lbp2900_wait_ready(state->ops);
}

static void lbp6000_job_prologue(struct printer_state_s *state)
{
	begin_job(state);

	capt_sendrecv(CAPT_GPIO, lbp3010_gpio_init, ARRAY_SIZE(lbp3010_gpio_init), NULL, 0);
	lbp2900_wait_ready(state->ops);

	capt_sendrecv(CAPT_LBP6000_SETUP_0, lbp6000_job_init, ARRAY_SIZE(lbp6000_job_init), NULL, 0);
	lbp2900_wait_ready(state->ops);

	send_job_start(1, 0);
	lbp2900_wait_ready(state->ops);
}

/*
 * Prepare the printer for one page: translate media type/size into CAPT codes,
 * wake the engine if it reports itself uninitialised, wait (bounded) for
 * buffer space, then send the page and Hi-SCoA parameters in one
 * CAPT_SET_PARMS packet. Always returns true.
 */
static bool lbp2900_page_prologue(struct printer_state_s *state, const struct page_dims_s *dims)
{
	const struct capt_status_s *status;
	size_t s;
	uint8_t buf[16];

	uint8_t sz = 0x00; /* page size */
	uint8_t save = dims->toner_save;
	uint8_t ink_k = (dims->ink_k<<2);
	uint8_t fm = 0x00; /* fuser mode (temperature?) */
	uint8_t air = 0x02; /* automatic image refinement */

	switch (dims->media_type) {
		case 0x00:
		case 0x02:
			/* Plain Paper & Plain Paper L */
			fm = 0x01;
			break;
		case 0x01:
			/* Thick Paper */
			fm = 0x01;
			break;
		case 0x03:
			/* Thick Paper H */
			fm = 0x02;
			break;
		case 0x04:
			/* Transparency */
			fm = 0x13;
			break;
		case 0x05:
			/* Transparency */
			fm = 0x14;
			break;
		case 0x06:
			/* Envelope */
			fm = 0x1C;
			break;
		default:
			fm = 0x01;
	}
	fprintf(stderr, "DEBUG: CAPT: media_type=%u, fm=%u\n", dims->media_type, fm);

	if ( strncmp(dims->media_size, "A4", 2) == 0 ) sz = 0x02;
	else if ( strncmp(dims->media_size, "A5", 2) == 0 ) sz = 0x03;
	else if ( strncmp(dims->media_size, "B5", 2) == 0 ) sz = 0x07;
	else if ( strncmp(dims->media_size, "Executive", 9) == 0 ) sz = 0x0A;
	else if ( strncmp(dims->media_size, "Legal", 5) == 0 ) sz = 0x0C;
	else if ( strncmp(dims->media_size, "Letter", 6) ==0 ) sz = 0x0D;
	else if ( strncmp(dims->media_size, "EnvC5", 5) ==0 ) sz = 0x15;
	else if ( strncmp(dims->media_size, "Env10", 5) == 0 ) sz = 0x16;
	else if ( strncmp(dims->media_size, "EnvMonarch", 10) == 0 ) sz = 0x17;
	else if ( strncmp(dims->media_size, "EnvDL", 5) == 0 ) sz = 0x18;
	else if ( strncmp(dims->media_size, "3x5", 3) ==0 ) sz = 0x40;
	else if ( strncmp(dims->media_size, "PRC16K", 6) ==0 ) sz = 0xD4;
	else sz = 0x02;
	fprintf(stderr, "DEBUG: CAPT: media_size=%s, sz=%u\n", dims->media_size, sz);

	uint8_t pageparms[] = {
		/* Bytes 0-21 (0x00 to 0x15) */
		0x00, 0x00, 0x30, 0x2A, sz, 0x00, 0x00, 0x00,
		ink_k, 0x1C, 0x1C, 0x1C, dims->media_type, dims->media_adapt, 0x04, 0x00,
		0x01, 0x01, air, save, 0x00, 0x00,
		/* Bytes 22-33 (0x16 to 0x21) */
		LO(dims->margin_height), HI(dims->margin_height),
		LO(dims->margin_width), HI(dims->margin_width),
		LO(dims->line_size), HI(dims->line_size),
		LO(dims->num_lines), HI(dims->num_lines),
		LO(dims->paper_width), HI(dims->paper_width),
		LO(dims->paper_height), HI(dims->paper_height),
		/* Bytes 34-39 (0x22 to 0x27) */
		0x00, 0x00, fm, 0x00, 0x00, 0x00,
		/* Spare bytes for later
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		*/
	};

	status = lbp2900_get_status(state->ops);
	if (FLAG(status, CAPT_FL_UNINIT1) || FLAG(status, CAPT_FL_UNINIT2)) {
		capt_sendrecv(CAPT_START_1, NULL, 0, NULL, 0);
		capt_sendrecv(CAPT_START_2, NULL, 0, NULL, 0);
		capt_sendrecv(CAPT_START_3, NULL, 0, NULL, 0);
		//lbp2900_get_status(state->ops);
		lbp2900_wait_ready(state->ops);

		capt_sendrecv(CAPT_UPLOAD_2, magicbuf_2, ARRAY_SIZE(magicbuf_2), NULL, 0);
		lbp2900_wait_ready(state->ops);
	}

	for (unsigned polls = 0; ; ++polls) {
		if (! FLAG(lbp2900_get_status(state->ops), CAPT_FL_BUFFERFULL))
			break;
		if (polls >= CAPT_WAIT_POLLS_PAGE) {
			fprintf(stderr, "WARNING: CAPT: buffer stayed full ~%us; proceeding\n",
					CAPT_WAIT_POLLS_PAGE / 10);
			break;
		}
		usleep(CAPT_POLL_US);
	}

	capt_multi_begin(CAPT_SET_PARMS);
	capt_multi_add(CAPT_SET_PARM_PAGE, pageparms, sizeof(pageparms));
	s = hiscoa_format_params(buf, sizeof(buf), &hiscoa_default_params);
	capt_multi_add(CAPT_SET_PARM_HISCOA, buf, s);
	capt_multi_add(CAPT_SET_PARM_1, NULL, 0);
	capt_multi_add(CAPT_SET_PARM_2, NULL, 0);
	capt_multi_send();

	return true;
}

/*
 * Finish one page: mark the end of the data, wait (bounded) until the printer
 * has received it, fire it, and wait until it is ejected.
 *
 * Returns false if the printer ran out of paper before printing the page; the
 * caller then waits for the user and resends it. While paper is missing,
 * "STATE: +media-empty" is reported to CUPS and the LED blinks.
 */
static bool lbp2900_page_epilogue(struct printer_state_s *state, const struct page_dims_s *dims)
{
	(void) dims;
	const struct capt_status_s *status;

	capt_send(CAPT_PRINT_DATA_END, NULL, 0);

	/* waiting until the page is received (check first, then poll finely so
	 * a ready printer isn't stalled a whole second before the next page) */
	for (unsigned polls = 0; ; ++polls) {
		status = lbp2900_get_status(state->ops);
		if (status->page_received == status->page_decoding)
			break;
		if (polls >= CAPT_WAIT_POLLS_PAGE) {
			fprintf(stderr, "WARNING: CAPT: page-received didn't settle ~%us; proceeding\n",
					CAPT_WAIT_POLLS_PAGE / 10);
			break;
		}
		usleep(CAPT_POLL_US);
	}
	send_job_start(2, status->page_decoding);
	lbp2900_wait_ready(state->ops);

	uint8_t buf[2] = { LO(status->page_decoding), HI(status->page_decoding) };
	capt_sendrecv(CAPT_FIRE, buf, 2, NULL, 0);
	lbp2900_wait_ready(state->ops);

	send_job_start(6, status->page_decoding);

	bool blinking = false;
	for (unsigned polls = 0; ; ++polls) {
		const struct capt_status_s *status = lbp2900_get_status(state->ops);
		/* Interesting. Using page_printing here results in shifted print */
		if (status->page_out == status->page_decoding) {
			/* Page ejected OK -> clear the out-of-paper reason. */
			fprintf(stderr, "STATE: -media-empty\n");
			return true;
		}
		if (FLAG(status, CAPT_FL_NOPAPER2) || FLAG(status, CAPT_FL_NOPAPER1)) {
			fprintf(stderr, "DEBUG: CAPT: no paper\n");
			/* Surface "Out of paper" in the macOS print queue window. */
			fprintf(stderr, "STATE: +media-empty\n");
			/* Blink the printer's LED so it's obvious at the machine. */
			if (! blinking) {
				capt_sendrecv(CAPT_GPIO, lbp2900_gpio_blink,
						ARRAY_SIZE(lbp2900_gpio_blink), NULL, 0);
				blinking = true;
			}
			if (FLAG(status, CAPT_FL_PRINTING) || FLAG(status, CAPT_FL_PROCESSING1))
				continue;
			return false;
		}
		if (polls >= CAPT_WAIT_POLLS_PAGE) {
			fprintf(stderr, "WARNING: CAPT: page-out didn't settle ~%us; continuing\n",
					CAPT_WAIT_POLLS_PAGE / 10);
			return true;
		}
		usleep(CAPT_POLL_US);
	}
}

/* End the job once every page is completed (or after the bounded wait). */
static void lbp2900_job_epilogue(struct printer_state_s *state)
{
	uint8_t jbuf[2] = { LO(job), HI(job) };
	const struct capt_status_s *status = NULL;

	for (unsigned polls = 0; ; ++polls) {
		status = lbp2900_get_status(state->ops);
		if (status->page_completed == status->page_decoding) {
			send_job_start(4, status->page_completed);
			break;
		}
		if (polls >= CAPT_WAIT_POLLS_JOB) {
			fprintf(stderr, "WARNING: CAPT: page-completed didn't settle ~%us; ending job\n",
					CAPT_WAIT_POLLS_JOB / 10);
			send_job_start(4, status->page_decoding);
			break;
		}
		usleep(CAPT_POLL_US);
	}
	capt_sendrecv(CAPT_JOB_END, jbuf, 2, NULL, 0);
}

/* No-op: page geometry now comes straight from the CUPS header (paper.c). */
static void lbp2900_page_setup(struct printer_state_s *state,
		struct page_dims_s *dims,
		unsigned width, unsigned height)
{
	/* FIXME: Do we still need this function? */
	(void) state;
	(void) width;
	(void) height;
	(void) dims;
	/* Get raster dimensions straight from CUPS in paper.c */
	//dims->num_lines = dims->paper_height;
	//dims->line_size = dims->paper_width / 8;
	//dims->band_size = 70;
}

/* Job cancelled: stop the LED blinking and close the job on the printer. */
static void lbp2900_cancel_cleanup(struct printer_state_s *state)
{
	const struct capt_status_s *status = lbp2900_get_status(state->ops);
	uint8_t jbuf[2] = { LO(job), HI(job) };

	capt_sendrecv(CAPT_GPIO, lbp2900_gpio_init, ARRAY_SIZE(lbp2900_gpio_init), NULL, 0);
	send_job_start(4, status->page_completed);
	capt_sendrecv(CAPT_JOB_END, jbuf, 2, NULL, 0);
}

/* As lbp2900_cancel_cleanup(), with the LBP3010-style GPIO payload. */
static void lbp3010_cancel_cleanup(struct printer_state_s *state)
{
	const struct capt_status_s *status = lbp2900_get_status(state->ops);
	uint8_t jbuf[2] = { LO(job), HI(job) };

	capt_sendrecv(CAPT_GPIO, lbp3010_gpio_init, ARRAY_SIZE(lbp3010_gpio_init), NULL, 0);
	send_job_start(4, status->page_completed);
	capt_sendrecv(CAPT_JOB_END, jbuf, 2, NULL, 0);
}

/*
 * Blink the LED and block until the user presses the printer's button
 * (used for manual duplex and to resume after a failed page).
 */
static void lbp2900_wait_user(struct printer_state_s *state)
{
	capt_sendrecv(CAPT_GPIO, lbp2900_gpio_blink, ARRAY_SIZE(lbp2900_gpio_blink), NULL, 0);
	lbp2900_wait_ready(state->ops);

	while (1) {
		const struct capt_status_s *status = lbp2900_get_status(state->ops);
		if (FLAG(status, CAPT_FL_BUTTON_ON)) {
			fprintf(stderr, "DEBUG: CAPT: button activated\n");
		}
		if (FLAG(status, CAPT_FL_BUTTON)) {
			fprintf(stderr, "DEBUG: CAPT: button pressed\n");
			break;
		}
		sleep(1);
	}

	capt_sendrecv(CAPT_GPIO, lbp2900_gpio_init, ARRAY_SIZE(lbp2900_gpio_init), NULL, 0);
	lbp2900_wait_ready(state->ops);
}

/* As lbp2900_wait_user(); these models report the button via CAPT_FL_nERROR. */
static void lbp3010_wait_user(struct printer_state_s *state)
{

	capt_sendrecv(CAPT_GPIO, lbp3010_gpio_blink, ARRAY_SIZE(lbp3010_gpio_blink), NULL, 0);
	lbp2900_wait_ready(state->ops);

	while (1) {
		const struct capt_status_s *status = lbp2900_get_status(state->ops);
		if (FLAG(status, CAPT_FL_BUTTON_ON)) {
			fprintf(stderr, "DEBUG: CAPT: button activated\n");
		}
		if (FLAG(status, CAPT_FL_nERROR)) {
			fprintf(stderr, "DEBUG: CAPT: (virtual) button pressed\n");
			break;
		}
		sleep(1);
	}

	capt_sendrecv(CAPT_GPIO, lbp3010_gpio_init, ARRAY_SIZE(lbp3010_gpio_init), NULL, 0);
	lbp2900_wait_ready(state->ops);
}

static struct lbp2900_ops_s lbp2900_ops = {
	.ops = {
		.job_prologue = lbp2900_job_prologue,
		.job_epilogue = lbp2900_job_epilogue,
		.page_setup = lbp2900_page_setup,
		.page_prologue = lbp2900_page_prologue,
		.page_epilogue = lbp2900_page_epilogue,
		.compress_band = ops_compress_band_hiscoa,
		.send_band = ops_send_band_hiscoa,
		.cancel_cleanup = lbp2900_cancel_cleanup,
		.wait_user = lbp2900_wait_user,
	},
	.gpio = {
		.init = lbp2900_gpio_init,
		.blink = lbp2900_gpio_blink,
	},
	/* LBP2900 fix: always fetch the *extended* status so the page_*
	 * counters (page_decoding/page_out/page_completed) are refreshed on
	 * every poll. The conditional capt_get_xstatus only escalates to
	 * CHKXSTATUS when XSTATUS_CHNG is set, which this unit never sets,
	 * leaving page_* stale/garbage and hanging the epilogue wait loops.
	 * This mirrors the known-working LBP3010 configuration. */
	.get_status = capt_get_xstatus_only,
	.wait_ready = capt_wait_xready_only,
};
register_printer("LBP2900", lbp2900_ops.ops, WORKS);

static struct lbp2900_ops_s lbp3000_ops = {
	.ops = {
		.job_prologue = lbp3000_job_prologue,	/* different job prologue */
		.job_epilogue = lbp2900_job_epilogue,
		.page_setup = lbp2900_page_setup,
		.page_prologue = lbp2900_page_prologue,
		.page_epilogue = lbp2900_page_epilogue,
		.compress_band = ops_compress_band_hiscoa,
		.send_band = ops_send_band_hiscoa,
		.cancel_cleanup = lbp2900_cancel_cleanup,
		.wait_user = lbp2900_wait_user,
	},
	.gpio = {
		.init = lbp2900_gpio_init,
		.blink = lbp2900_gpio_blink,
	},
	.get_status = capt_get_xstatus,
	.wait_ready = capt_wait_ready,
};
register_printer("LBP3000", lbp3000_ops.ops, EXPERIMENTAL);

static struct lbp2900_ops_s lbp3010_ops = {
	.ops = {
		.job_prologue = lbp3010_job_prologue,
		.job_epilogue = lbp2900_job_epilogue,
		.page_setup = lbp2900_page_setup,
		.page_prologue = lbp2900_page_prologue,
		.page_epilogue = lbp2900_page_epilogue,
		.compress_band = ops_compress_band_hiscoa,
		.send_band = ops_send_band_hiscoa,
		.cancel_cleanup = lbp3010_cancel_cleanup,
		.wait_user = lbp3010_wait_user,
	},
	.gpio = {
		.init = lbp3010_gpio_init,
		.blink = lbp3010_gpio_blink,
	},
	.get_status = capt_get_xstatus_only,
	.wait_ready = capt_wait_xready_only,
};

static struct lbp2900_ops_s lbp6000_ops = {
	.ops = {
		.job_prologue = lbp6000_job_prologue,
		.job_epilogue = lbp2900_job_epilogue,
		.page_setup = lbp2900_page_setup,
		.page_prologue = lbp2900_page_prologue,
		.page_epilogue = lbp2900_page_epilogue,
		.compress_band = ops_compress_band_hiscoa,
		.send_band = ops_send_band_hiscoa,
		.cancel_cleanup = lbp3010_cancel_cleanup,
		.wait_user = lbp3010_wait_user,
	},
	.gpio = {
		.init = lbp3010_gpio_init,
		.blink = lbp3010_gpio_blink,
	},
	.get_status = capt_get_xstatus_only,
	.wait_ready = capt_wait_xready_only,
};

register_printer("LBP3010/LBP3018/LBP3050", lbp3010_ops.ops, WORKS);
register_printer("LBP3100/LBP3108/LBP3150", lbp3010_ops.ops, EXPERIMENTAL);
register_printer("LBP6000/LBP6018", lbp6000_ops.ops, EXPERIMENTAL);
