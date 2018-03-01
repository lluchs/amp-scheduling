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

/* micro with manual Ryzen L3 cache miss counter monitoring. */

#include <stdio.h>

#include "ultmigration.h"
#include "tools/msrtools.h"
#include "pmc/pmc.h"
#include "micro.h"

// L3 counter indices
enum {
	L3Ctr_Fast,
	L3Ctr_Slow,
};

static void pmc_init(int fast_cpu, int slow_cpu) {
	// Count cache misses for both cores/threads separately.
	union pmc_l3_event event = {
		.EventSel = L3Miss,
		.UnitMask = L3MissUMask,
		.Enable = 1,
		.SliceMask = 0xf,
		.ThreadMask = 0xff,
	};
	pmc_select_l3_event(fast_cpu, L3Ctr_Fast, event);
	pmc_select_l3_event(slow_cpu, L3Ctr_Slow, event);
}

// Saturating a += b for unsigned integers;
#define SAT_INC(a, b) do { (a) += (b); if ((a) < (b)) (a) = -1; } while (0)

// Read counter, doing saturating addition to detect overflows in the result.
static inline void read_l3_counter(int ctr, uint64_t *var) {
	uint64_t counter = pmc_read_l3_counter(ctr);
	SAT_INC(*var, counter);
	// Check the counter's overflow bit.
	if (counter & ((uint64_t)1 << 48)) *var = -1;
	pmc_write_l3_counter(PMC_CURRENT_CPU, ctr, 0);
}

extern int slow_cpu, fast_cpu;

void run_benchmark(Fn littlework, Fn lotsofwork) {
	init_dev_msr();
	pmc_init(fast_cpu, slow_cpu);
	uint64_t cache_misses_fast = 0, cache_misses_slow = 0;
	uint64_t instructions_fast = 0, instructions_slow = 0, instructions;

	int result = 0;
	for (int i = 0; i < WORK; i++) {
		ult_migrate(ULT_FAST);
		pmc_write_l3_counter(PMC_CURRENT_CPU, L3Ctr_Fast, 0);
		instructions = pmc_read_ir(PMC_CURRENT_CPU);
		result += lotsofwork();
		instructions = pmc_read_ir(PMC_CURRENT_CPU) - instructions;
		SAT_INC(instructions_fast, instructions);
		read_l3_counter(L3Ctr_Fast, &cache_misses_fast);

		ult_migrate(ULT_SLOW);
		pmc_write_l3_counter(PMC_CURRENT_CPU, L3Ctr_Slow, 0);
		instructions = pmc_read_ir(PMC_CURRENT_CPU);
		result += littlework();
		instructions = pmc_read_ir(PMC_CURRENT_CPU) - instructions;
		SAT_INC(instructions_slow, instructions);
		read_l3_counter(L3Ctr_Slow, &cache_misses_slow);
	}
	printf("done %d\n", result);

	printf("Instructions    (Fast): %"PRIu64"%s\n", instructions_fast, instructions_fast == -1 ? " overflow!" : "");
	printf("         per iteration: %"PRIu64"\n", instructions_fast / WORK);
	printf("Instructions    (Slow): %"PRIu64"%s\n", instructions_slow, instructions_slow == -1 ? " overflow!" : "");
	printf("         per iteration: %"PRIu64"\n", instructions_slow / WORK);
	printf("L3 Cache Misses (Fast): %"PRIu64"%s\n", cache_misses_fast, cache_misses_fast == -1 ? " overflow!" : "");
	printf("         per iteration: %"PRIu64"\n", cache_misses_fast / WORK);
	printf("L3 Cache Misses (Slow): %"PRIu64"%s\n", cache_misses_slow, cache_misses_slow == -1 ? " overflow!" : "");
	printf("         per iteration: %"PRIu64"\n", cache_misses_slow / WORK);
	printf("Miss / 1k Instr (Fast): %Lf\n", (long double) cache_misses_fast / (long double) instructions_fast * 1000);
	printf("Miss / 1k Instr (Slow): %Lf\n", (long double) cache_misses_slow / (long double) instructions_slow * 1000);
}
