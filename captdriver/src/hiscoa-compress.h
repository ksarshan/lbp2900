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
 * hiscoa-compress.h - Hi-SCoA raster band compressor.
 */

#pragma once

#include "std.h"
#include "hiscoa-common.h"

/*
 * Compress @nlines rows of @line_size bytes from @band into @buf.
 *
 * @size is the capacity of @buf; output that does not fit is silently
 * truncated, so callers should provide a generous buffer (rastertocapt uses
 * twice the raw band size). The output is padded to a 32-bit boundary and
 * terminated with @eob_type.
 *
 * Returns the number of bytes written.
 */
size_t hiscoa_compress_band(void *buf, size_t size,
	const void *band, unsigned line_size, unsigned nlines,
	enum hiscoa_eob_type eob_type,
	const struct hiscoa_params *params);

/*
 * Serialise @params into the 8-byte payload of CAPT_SET_PARM_HISCOA.
 * Returns the number of bytes written, or 0 if @size is too small.
 */
size_t hiscoa_format_params(void *buf, size_t size,
	const struct hiscoa_params *params);
