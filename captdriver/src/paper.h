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
 * paper.h - per-page geometry and media settings.
 */

#pragma once

#include "std.h"

struct cups_page_header2_s;

/* Page settings for one page, taken from its CUPS raster header. */
struct page_dims_s {
	/* set by CUPS */
	unsigned media_type;	/* cupsMediaType: plain, thick, envelope... (see canon-lbp.drv) */
	unsigned media_adapt;	/* resolution-dependent CAPT flag: 0x81 at 400 dpi, else 0x11 */
	char media_size[0x40];	/* PPD media name, e.g. "A4", "Letter" */
	unsigned paper_width;	/* image width in pixels */
	unsigned paper_height;	/* image height in pixels */
	unsigned toner_save;	/* cupsInteger[0]: non-zero enables toner save */
	unsigned manual_duplex;	/* cupsInteger[2]: pause for the user between pages */
	unsigned ink_k;		/* cupsInteger[1] */
	unsigned margin_height;
	unsigned margin_width;
	/* raster layout */
	unsigned line_size;	/* bytes per scan line sent to the printer */
	unsigned band_size;	/* scan lines per compressed band */
	unsigned num_lines;	/* total scan lines on the page */
};

/* Fill @dims from a CUPS raster page header. */
void page_set_dims(struct page_dims_s *dims, const struct cups_page_header2_s *header);
