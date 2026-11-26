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
 * printer.h - printer model registry and the per-model operations table.
 *
 * Each model file defines a struct printer_ops_s and registers it with
 * register_printer(). At job start printer_detect() reads the printer's
 * IEEE 1284 device ID and returns the ops registered for its MODEL/MDL field.
 */

#pragma once

#include "std.h"

/* How well a registered model is known to work. */
enum printer_support {
	UNSUPPORTED,	/* refuse to print */
	BROKEN,		/* print, but log a warning */
	EXPERIMENTAL,	/* print, but log a warning */
	WORKS,
	FULLY,
};

struct page_dims_s;

/* Per-job state shared between rastertocapt.c and the model ops. */
struct printer_state_s {
	const struct printer_ops_s *ops;
	unsigned ipage;	/* 1-based number of the current page */
	unsigned iband;	/* index of the band being sent within the page */
	unsigned isend;	/* number of print-data packets sent for the page */
};

/*
 * Model-specific callbacks, called by rastertocapt.c in this order:
 *
 *   alloc_state                         once (optional)
 *   per page:
 *     page_setup, compress_band         (once per band; whole page cached)
 *     job_prologue                      only before the first page sent
 *     page_prologue, send_band          (once per band), page_epilogue
 *   job_epilogue                        once, after the last page
 *   free_state                          once (optional)
 *
 * page_prologue/page_epilogue return false when the page could not be
 * printed (e.g. out of paper); wait_user is then called and the same page is
 * retried. cancel_cleanup is called from the SIGTERM handler.
 */
struct printer_ops_s {
	struct printer_state_s * (*alloc_state) (void);
	void (*free_state) (struct printer_state_s *state);
	void (*job_prologue) (struct printer_state_s *state);
	void (*job_epilogue) (struct printer_state_s *state);
	void (*page_setup) (struct printer_state_s *state,
		struct page_dims_s *dims, unsigned width, unsigned height);
	bool (*page_prologue) (struct printer_state_s *state, const struct page_dims_s *dims);
	bool (*page_epilogue) (struct printer_state_s *state, const struct page_dims_s *dims);
	size_t (*compress_band) (struct printer_state_s *state,
		void *band, size_t size,
		const void *pixels, unsigned line_size, unsigned num_lines);
	void (*send_band) (struct printer_state_s *state, const void *band, size_t size);
	void (*cancel_cleanup) (struct printer_state_s *state);
	void (*wait_user) (struct printer_state_s *state);
};

/*
 * Identify the connected printer and return its ops.
 * Exits the filter with an error if the model is unknown or unsupported.
 */
const struct printer_ops_s *printer_detect(void);

/* Add a model to the registry. Use register_printer() instead of calling
 * this directly. */
void __printer_register_ops(const char *name, const struct printer_ops_s *ops,
		enum printer_support status);

/*
 * register_printer(model, ops, status) - register @ops for the IEEE 1284
 * model name @model at program start-up.
 *
 * Expands to a uniquely named constructor function (the name includes
 * __LINE__), so it may be used several times in one file. The trailing
 * prototype lets the macro be followed by a semicolon.
 */
#define ___concat(text, line) text##line
#define __printer_register_func(line) \
	___concat(static void __register_printer_, line)(void)

#define register_printer(mdl, ops, status) \
	__attribute__((constructor(150))) \
	__printer_register_func(__LINE__) \
	{ __printer_register_ops(mdl, &ops, status); } \
	__printer_register_func(__LINE__)
