/*
 * Copyright © 2017, Lukas Werling
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef MICRO_H
#define MICRO_H

/* Configuration */

// Multiplier for overall iterations.
// Increasing this number makes core switches more frequent while keeping the
// overall amount of work constant.
#define OUTER_WORK_MULT 1

// Overall iterations.
#define WORK (10000 * OUTER_WORK_MULT)
// Number of CPU work iterations.
#define CPU_ITERATIONS (400000 / OUTER_WORK_MULT)

// Size of memory buffer used for indirect_access and pointer_chase
#define BUFSIZE (size_t) (100 << 20)

// indirect_access: Length of access pattern.
#define PATTERN_LENGTH (100000 / OUTER_WORK_MULT)
// pointer_chasing: Number of steps per iteration.
#define POINTER_CHASE_STEPS (7000 / OUTER_WORK_MULT)

/* Declarations */

typedef int (*Fn)();
void run_benchmark(Fn littlework, Fn lotsofwork);

#endif
