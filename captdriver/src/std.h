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
 * std.h - common includes and helper macros shared by every driver source file.
 *
 * Include this first in each .c file: it sets the POSIX feature level before
 * any system header is pulled in.
 */

#pragma once

#define _POSIX_C_SOURCE 199309L /* try removing this if build fails */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Number of elements in a fixed-size array (not a pointer). */
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

/* Given a pointer to @member embedded in a @type, return the enclosing @type.
 * Used to recover model-specific ops from a generic struct printer_ops_s. */
#define container_of(ptr, type, member) \
	((type *) ((char *) ptr - offsetof(type, member)))

/* When true, every CAPT packet sent/received is hex-dumped to the CUPS log
 * as DEBUG: lines (only visible with `cupsctl LogLevel=debug`). */
#define debug true

/* Status-poll interval for the printer wait loops (microseconds). The stock
 * driver polled once per second (sleep(1)), which idles the print engine for up
 * to ~1 s at every handshake and makes multi-page jobs stall between sheets.
 * Polling at 100 ms keeps the printer fed and prints near-continuously, without
 * changing the CAPT command sequence. usleep() needs <unistd.h>. */
#define CAPT_POLL_US 100000
