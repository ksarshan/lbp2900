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
 * capt-command.h - CAPT packet transport.
 *
 * Every CAPT packet is a 4-byte header followed by an optional payload:
 *
 *   bytes 0-1  command code (little-endian, see enum capt_command)
 *   bytes 2-3  total packet length including the header (little-endian)
 *
 * Packets go to the printer on stdout (the CUPS usb backend forwards them)
 * and replies come back on the CUPS back-channel. Names marked "?" are
 * guesses from USB captures; see ../SPECS for the protocol notes.
 */

#pragma once

#include "std.h"

enum capt_command {
	CAPT_NOP        = 0xA0A0, /* ? */
	CAPT_CHKJOBSTAT = 0xA0A1,
	CAPT_CHKXSTATUS = 0xA0A8,

	CAPT_IEEE_IDENT = 0xA1A0, /* raw reply */
	CAPT_IDENT      = 0xA1A1,

	CAPT_JOB_BEGIN  = 0xA2A0,

	CAPT_START_0    = 0xA3A2,

	CAPT_PRINT_DATA     = 0xC0A0,
	CAPT_PRINT_DATA_END = 0xC0A4,

	CAPT_SET_PARM_PAGE   = 0xD0A0,
	CAPT_SET_PARM_1      = 0xD0A1,
	CAPT_SET_PARM_2      = 0xD0A2,
	CAPT_SET_PARM_HISCOA = 0xD0A4,
	CAPT_SET_PARMS  = 0xD0A9,

	CAPT_CHKSTATUS  = 0xE0A0,
	CAPT_START_2    = 0xE0A2,
	CAPT_START_1    = 0xE0A3,
	CAPT_START_3    = 0xE0A4,
	CAPT_UPLOAD_2   = 0xE0A5,
	CAPT_LBP3000_SETUP_0 = 0xE0A6,
	CAPT_FIRE       = 0xE0A7,
	CAPT_JOB_END    = 0xE0A9,
	CAPT_LBP6000_SETUP_0 = 0xE0BA,

	CAPT_JOB_SETUP  = 0xE1A1,
	CAPT_GPIO       = 0xE1A2,	/* LED / panel control */
	CAPT3_UNK_0     = 0xE0BA,	/* same code as CAPT_LBP6000_SETUP_0 */
};

/*
 * Return the printer's IEEE 1284 device ID string
 * ("MFG:Canon;MDL:LBP2900;..."), retrying until it is non-empty.
 * The string lives in the shared I/O buffer and is overwritten by the next
 * CAPT call. Exits the filter if the backend cannot be reached.
 */
const char *capt_identify(void);

/* Send one packet and do not wait for a reply. @data may be NULL. */
void capt_send(uint16_t cmd, const void *data, size_t size);

/*
 * Send one packet and read the printer's reply, which must echo @cmd.
 *
 * If @reply is non-NULL, the reply (without its 4-byte header) is copied into
 * it. On entry *@reply_size is the capacity of @reply in bytes; on return it
 * holds the total reply length *including* the 4-byte header. Pass a NULL
 * @reply_size only when @reply is NULL too. Exits the filter on a malformed
 * reply or timeout.
 */
void capt_sendrecv(uint16_t cmd, const void *buf, size_t size, void *reply, size_t *reply_size);

/*
 * Compound packets: capt_multi_begin() starts a container packet @cmd,
 * capt_multi_add() appends sub-packets to it, and capt_multi_send() fills in
 * the total length and sends it. Used for CAPT_SET_PARMS.
 */
void capt_multi_begin(uint16_t cmd);
void capt_multi_add(uint16_t cmd, const void *data, size_t size);
void capt_multi_send(void);
