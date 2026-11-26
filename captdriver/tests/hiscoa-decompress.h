/*
 * hiscoa-decompress.h - reference Hi-SCoA decompressor (tests only).
 */

#pragma once

#include "../src/std.h"
#include "../src/hiscoa-common.h"

/*
 * Decompress one band. On entry *@band and *@size describe the compressed input
 * and *@output_size the capacity of @output. On return *@band and *@size are
 * advanced past the consumed input and *@output_size holds the number of
 * bytes produced.
 */
unsigned hiscoa_decompress_band(
	const void **band, size_t *size,
	void *output, size_t *output_size,
	unsigned line_size,
	const struct hiscoa_params *params
);
