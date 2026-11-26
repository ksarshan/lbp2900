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
 * capt-status.h - printer status polling.
 *
 * Two status queries exist:
 *   CAPT_CHKSTATUS   basic status: only status[0] (busy/ready flags) is reliable.
 *   CAPT_CHKXSTATUS  extended status: all status words plus the page counters.
 *
 * The page counters are cumulative, so the driver waits for one counter to
 * catch up with another (e.g. page_out == page_decoding) rather than for a
 * specific value.
 */

#pragma once

#include "std.h"

/* Last status reported by the printer. */
struct capt_status_s {
	uint16_t status[7];	/* raw status words; decode with FLAG() */

	uint16_t page_decoding;	/* pages accepted for decoding */
	uint16_t page_printing;	/* pages being printed */
	uint16_t page_out;	/* pages ejected */
	uint16_t page_completed;/* pages finished */
	uint16_t page_received;	/* pages whose data has been fully received */
};

/*
 * Status flags. Each value packs the status[] word index in the upper 16 bits
 * and the bit mask in the lower 16 bits; test them with FLAG().
 * Names marked "?" are not fully understood.
 */
#define _FL(s, b) ((s << 16) | (1 << b))
enum capt_flags
{
	/* status[0] */
	CAPT_FL_READY1       = _FL(0, 15), /* ? */
	CAPT_FL_READY2       = _FL(0, 12), /* ? */
	CAPT_FL_JOBSTAT_CHNG = _FL(0, 9),
	CAPT_FL_XSTATUS_CHNG = _FL(0, 8),
	CAPT_FL_BUSY         = _FL(0, 7),
	CAPT_FL_UNINIT1      = _FL(0, 5),
	CAPT_FL_UNINIT2      = _FL(0, 4),
	CAPT_FL_BUFFERFULL   = _FL(0, 2),
	CAPT_FL_NOPAPER1     = _FL(0, 1),
	CAPT_FL_PROCESSING   = _FL(0, 0),
	/* status[1] */
	CAPT_FL_NOPAPER2     = _FL(1, 14),
	CAPT_FL_PROCESSING1  = _FL(1, 7),
	CAPT_FL_BUTTON       = _FL(1, 5),
	CAPT_FL_PRINTING     = _FL(1, 2),
	CAPT_FL_POWERUP      = _FL(1, 0),
	/* status[2] */
	CAPT_FL_nERROR       = _FL(2, 7),
	CAPT_FL_BUTTON1      = _FL(2, 8),
	/* status[3] */
	CAPT_FL_POWERUP1     = _FL(3, 12),
	/* status[4] */
	CAPT_FL_BUTTON_ON    = _FL(4, 0),
	/* status[5] */
	/* status[6] */
};
#undef _FL

/* True if @flag is set in @status. */
static inline bool FLAG(const struct capt_status_s *status, enum capt_flags flag)
{
	return !! (status->status[flag >> 16] & (flag & 0xFFFF));
}

/* Reset the cached status to all zeros (call at job start). */
void capt_init_status(void);

/*
 * The getters below query the printer and return a pointer to one shared,
 * cached struct that every later call overwrites.
 */

/* Basic status (CAPT_CHKSTATUS). */
const struct capt_status_s *capt_get_status(void);
/* Extended status (CAPT_CHKXSTATUS), every time. Used by the LBP2900. */
const struct capt_status_s *capt_get_xstatus_only(void);
/* Basic status, followed by extended status only if the printer sets
 * CAPT_FL_XSTATUS_CHNG. */
const struct capt_status_s *capt_get_xstatus(void);

/* Poll the matching getter every CAPT_POLL_US until CAPT_FL_BUSY clears. */
void capt_wait_ready(void);
void capt_wait_xready(void);
void capt_wait_xready_only(void);
