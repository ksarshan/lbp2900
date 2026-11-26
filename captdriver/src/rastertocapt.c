/*
 * Copyright (C) 2013 Alexey Galakhov <agalakhov@gmail.com>
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
 * rastertocapt.c - CUPS filter entry point: CUPS raster in, CAPT out.
 *
 * CUPS runs this as:  rastertocapt job-id user title copies options [file]
 * It reads CUPS raster from @file (or stdin), writes CAPT packets to stdout
 * for the usb backend, and logs to stderr using the CUPS message prefixes
 * (DEBUG:, INFO:, ERROR:, PAGE:, STATE:).
 *
 * Each page is fully compressed into memory before anything is sent, so a
 * page that fails (e.g. out of paper) can be resent without re-reading the
 * raster stream.
 */

#include "std.h"
#include "printer.h"
#include "paper.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cups/raster.h>


/* A page compressed into memory, ready to send (and resend). */
struct cached_page_s {
	struct page_dims_s dims;
	struct band_list_s *bands;	/* compressed bands, in print order */
};

/* One compressed band; @data holds @size bytes. */
struct band_list_s {
	struct band_list_s *next;
	size_t size;
	uint8_t data[];
};

/* Printer and job state. File-scope so the SIGTERM handler can clean up. */
static const struct printer_ops_s *ops;
static struct printer_state_s *state = NULL;
static struct cached_page_s *cached_page = NULL;
static cups_raster_t *raster;

/* Scratch buffers used while compressing one page. */
static uint8_t *linebuf = NULL;
static uint8_t *bandbuf = NULL;
static uint8_t *compbuf = NULL;

/* Allocation size for a band_list_s carrying @size bytes of data. */
static inline size_t sizeof_struct_band_list_s(size_t size)
	{ return sizeof(struct band_list_s) + size; }

/*
 * Byte offset that centres a line of @small bytes inside one of @large bytes,
 * rounded to whole pixels of @bpp bits.
 */
static size_t center_pixels(size_t small, size_t large, unsigned bpp)
{
	unsigned small_p;
	unsigned large_p;
	unsigned bypp;
	while (bpp % 8)
		bpp *= 2;
	bypp = bpp / 8;
	small_p = small / bypp;
	large_p = large / bypp;
	return bypp * ((large_p - small_p) / 2);
}

static void free_cached_page(struct cached_page_s *cached_page)
{
	while (cached_page->bands) {
		void *p = cached_page->bands;
		cached_page->bands = cached_page->bands->next;
		free(p);
	}
	free(cached_page);
}

static void free_state(void)
{
	if (state) {
		if (ops->free_state)
			ops->free_state(state);
		else
			free(state);
		state = NULL;
	}
}

static void free_buffers(void)
{
	if (compbuf) {
		free(compbuf);
		compbuf = NULL;
	}
	if (bandbuf) {
		free(bandbuf);
		bandbuf = NULL;
	}
	if (linebuf) {
		free(linebuf);
		linebuf = NULL;
	}
}

/*
 * Read one page of raster from @raster, fit each line to the printer's line
 * size (centring narrower lines, cropping wider ones), and compress it band by
 * band into @page->bands. Extra lines beyond the printable height are read
 * and discarded so the raster stream stays in sync.
 */
static void compress_page_data(struct printer_state_s *state,
		struct cached_page_s *page,
		cups_raster_t *raster,
		const struct cups_page_header2_s *header)
{
	const struct page_dims_s *dims = &page->dims;
	const unsigned compsize = 2 * dims->line_size * dims->band_size;

	struct band_list_s *last_band = NULL;
	unsigned i;
	unsigned iband;

	unsigned shiftb = 0;
	unsigned shiftl = 0;
	unsigned csize = header->cupsBytesPerLine;


	if (header->cupsBytesPerLine < dims->line_size) {
		csize = header->cupsBytesPerLine;
		shiftb = center_pixels(header->cupsBytesPerLine, dims->line_size,
				header->cupsBitsPerPixel);
	}

	if (dims->line_size < header->cupsBytesPerLine) {
		csize = dims->line_size;
		shiftl = center_pixels(dims->line_size, header->cupsBytesPerLine,
				header->cupsBitsPerPixel);
	}

	if (header->cupsBytesPerLine > dims->line_size)
		fprintf(stderr, "INFO: cutting line from %u to %u bytes\n",
				header->cupsBytesPerLine, dims->line_size);

	if (header->cupsHeight > dims->num_lines)
		fprintf(stderr, "INFO: cutting image from %u to %u lines\n",
				header->cupsHeight, dims->num_lines);

	linebuf = calloc(1, header->cupsBytesPerLine);
	if (! linebuf)
		abort();
	bandbuf = calloc(dims->line_size, dims->band_size);
	if (! bandbuf)
		abort();
	compbuf = calloc(1, compsize);
	if (! compbuf)
		abort();

	for (iband = 0; iband * dims->band_size < dims->num_lines; ++iband) {
		struct band_list_s *new_band;
		unsigned start = iband * dims->band_size;
		unsigned nlines = dims->band_size;
		unsigned iline;
		size_t size;
		memset(bandbuf, 0, dims->line_size * dims->band_size);
		if (nlines + start >= dims->num_lines)
			nlines = dims->num_lines - start;
		for (iline = 0; iline < nlines; ++iline) {
			if (iline + start < header->cupsHeight) {
				if (cupsRasterReadPixels(raster, linebuf, header->cupsBytesPerLine)
								!= header->cupsBytesPerLine) {
					fprintf(stderr, "ERROR: CAPT: broken raster file\n");
					exit(1);
				}
			} else {
				memset(linebuf, 0, header->cupsBytesPerLine);
			}
			memcpy(bandbuf + iline * dims->line_size + shiftb, linebuf + shiftl, csize);
		}
		size = state->ops->compress_band(state, compbuf, compsize, bandbuf, dims->line_size, nlines);
		new_band = calloc(1, sizeof_struct_band_list_s(size));
		if (! new_band)
			abort();
		new_band->size = size;
		if (size)
			memcpy(new_band->data, compbuf, size);
		if (! page->bands)
			page->bands = new_band;
		if (last_band)
			last_band->next = new_band;
		last_band = new_band;
	}

	/* discard end of image, if any */
	for (i = dims->num_lines; i < header->cupsHeight; ++i)
		cupsRasterReadPixels(raster, linebuf, header->cupsBytesPerLine);

	free_buffers();
}

/* Send every compressed band of @page to the printer. */
static void send_page_data(struct printer_state_s *state, const struct cached_page_s *page)
{
	const struct band_list_s *band;

	state->isend = 0;
	state->iband = 0;

	for (band = page->bands; band; band = band->next, ++state->iband)
		state->ops->send_band(state, band->data, band->size);
}

/*
 * SIGTERM handler: CUPS sends SIGTERM when the user cancels the job. Tell the
 * printer to end the job, free everything and exit.
 *
 * Note: this calls functions that are not async-signal-safe (stdio, malloc,
 * CUPS I/O). That works in practice because the filter is single-threaded,
 * but it is a known limitation of the upstream design.
 */
static void do_cancel(int s)
{
	(void) s;

	if (ops)
		ops->cancel_cleanup(state);

	if (raster) {
		cupsRasterClose(raster);
		raster = NULL;
	}

	free_buffers();

	if (cached_page) {
		free_cached_page(cached_page);
		cached_page = NULL;
	}

	if (state)
		free_state();

	exit(1);
}

/*
 * Print every page in the raster stream on @fd.
 *
 * A page whose prologue or epilogue fails (e.g. out of paper) stays cached:
 * the filter waits for the user to press the printer's button, then resends
 * the same page.
 */
static void do_print(int fd)
{
	bool in_job = false;
	ops = printer_detect();

	if (ops->alloc_state)
		state = ops->alloc_state();
	else
		state = calloc(1, sizeof(struct printer_state_s));
	state->ops = ops;
	state->ipage = 0;

	raster = cupsRasterOpen(fd, CUPS_RASTER_READ);

	fprintf(stderr, "DEBUG: CAPT: rastertocapt is rendering\n");

	while (1) {
		if (! cached_page) {
			struct cups_page_header2_s header;

			if (! cupsRasterReadHeader2(raster, &header))
				break; /* no more pages */

			cached_page = calloc(1, sizeof(struct cached_page_s));
			if (! cached_page)
				abort();

			state->ipage += 1;

			page_set_dims(&cached_page->dims, &header);

			ops->page_setup(state, &cached_page->dims,
					header.cupsWidth, header.cupsHeight);

			compress_page_data(state, cached_page, raster, &header);
		}

		if (! in_job) {
			fprintf(stderr, "DEBUG: CAPT: rastertocapt: start job\n");
			if (ops->job_prologue)
				ops->job_prologue(state);
			in_job = true;
		}

		fprintf(stderr, "DEBUG: CAPT: rastertocapt: start page %u\n", state->ipage);
		if (ops->page_prologue) {
			if (cached_page->dims.manual_duplex && state->ipage > 1) {
				fprintf(stderr, "DEBUG: CAPT: rastertocapt: manual duplex: press button to continue\n");
				ops->wait_user(state);
			}
			bool ok = ops->page_prologue(state, &cached_page->dims);
			if (! ok) {
				fprintf(stderr, "DEBUG: CAPT: rastertocapt: can't start page\n");
				ops->wait_user(state);
				if (in_job) {
					fprintf(stderr, "DEBUG: CAPT: rastertocapt: retry job\n");
					if (ops->job_epilogue)
						ops->job_epilogue(state);
					in_job = false;
				}
				continue;
			}
		}

		fprintf(stderr, "DEBUG: CAPT: rastertocapt: sending page data\n");
		/* PAGE: silently drives CUPS job-media-sheets-completed, which
		 * progress.sh reads for an accurate live page count. We deliberately
		 * do NOT emit an INFO: "Printing page N" string: on the macOS app print
		 * path (Word/Excel etc.) PrintCore writes its own "N of M" text into the
		 * same job-printer-state-message, so a second writer just produces a
		 * jumbled, flickering status line. */
		fprintf(stderr, "PAGE: %u 1\n", state->ipage);
		send_page_data(state, cached_page);

		fprintf(stderr, "DEBUG: CAPT: rastertocapt: end page %u\n", state->ipage);
		if (ops->page_epilogue) {
			bool ok = ops->page_epilogue(state, &cached_page->dims);
			if (! ok) {
				fprintf(stderr, "DEBUG: CAPT: rastertocapt: page not printed\n");
				ops->wait_user(state);
				continue;
			}
		}

		/* Page printed: drop it so the next iteration reads a new one. */
		free_cached_page(cached_page);
		cached_page = NULL;
	}

	if (in_job) {
		fprintf(stderr, "DEBUG: CAPT: rastertocapt: end job\n");
		if (ops->job_epilogue)
			ops->job_epilogue(state);
		in_job = false;
	}

	if (! state->ipage)
		fprintf(stderr, "ERROR: CAPT: no pages in job\n");

	cupsRasterClose(raster);
	free_state();
}


int main(int argc, char *argv[])
{
	/* A closed backend pipe must not kill the filter; CUPS cancels with SIGTERM. */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, do_cancel);

	int fd = 0;

	if (argc < 6 || argc > 7) {
		fprintf(stderr, "Usage: %s job-id user title copies options [file]\n", argv[0]);
		return 1;
	}

	if (argc == 7) {
		fd = open(argv[6], O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "ERROR: unable to open %s: %s\n", argv[6], strerror(errno));
			return 1;
		}
	}

	fprintf(stderr, "DEBUG: CAPT: rastertocapt started\n");
	do_print(fd);
	fprintf(stderr, "DEBUG: CAPT: rastertocapt finished\n");

	if (argc == 7)
		close(fd);

	return 0;
}
