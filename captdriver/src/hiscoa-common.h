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
 * hiscoa-common.h - types shared by the Hi-SCoA compressor and the
 * test-suite decompressor.
 *
 * Hi-SCoA ("High-performance Smart Compression Architecture") is Canon's
 * bit-oriented raster compression used by CAPT printers. See ../SPECS.
 */

#pragma once

#include "std.h"

/* Marker written after the last code of a band. */
enum hiscoa_eob_type {
	HISCOA_EOB_NORMAL = 0x0,	/* more bands follow */
	HISCOA_EOB_LAST = 0x01,		/* last band of the page */
};

/*
 * Back-reference "origins" for the long-repeat codes: each one is a distance
 * (in bytes) back from the current position where a repeated run may start.
 * origin_0 and origin_2 are relative to the line size, i.e. they point into
 * the previous scan line. The same values are sent to the printer with
 * CAPT_SET_PARM_HISCOA (see hiscoa_format_params()) so both sides agree.
 */
struct hiscoa_params {
	int origin_3;
	int origin_5;
	int origin_0;
	int origin_2;
	int origin_4;
};

/* Parameters used for every page this driver prints. */
extern const struct hiscoa_params hiscoa_default_params;
