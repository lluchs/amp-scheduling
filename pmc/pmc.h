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

#ifndef PMC_H
#define PMC_H

#include <inttypes.h>
#include <stdbool.h>

// Ryzen PPR, 2.1.13.4.1 L3 Cache PMC Events
enum L3PMC {
	L3Access = 0x01,
	L3AccessUMask = 0x80,
	L3Miss = 0x06,
	L3MissUMask = 0x01,
};

union pmc_l3_event {
	struct {
		uint64_t EventSel:8;
		uint64_t UnitMask:8;
		uint64_t _reserved1:6;
		uint64_t Enable:1;
		uint64_t _reserved2:25;
		uint64_t SliceMask:4;
		uint64_t _reserved3:4;
		uint64_t ThreadMask:8;
	};
	uint64_t value;
};
//_Static_assert(sizeof(union pmc_l3_event) == 8, "pmc_l3_event has correct size");

#define PMC_CURRENT_CPU -1

// Read the read-only instructions retired counter.
uint64_t pmc_read_ir(int cpu);

void pmc_select_l3_event(int cpu, uint8_t ctr, union pmc_l3_event);
void pmc_write_l3_counter(int cpu, uint8_t ctr, uint64_t value);

#if USE_PMC
inline uint64_t pmc_read_l3_counter(uint8_t ctr) {
	asm volatile("" ::: "memory");
	return __builtin_ia32_rdpmc(0xa + ctr);
}
#else
uint64_t pmc_read_l3_counter(uint8_t ctr);
#endif

// Converts a CPU number to a ThreadMask field for L3 events.
// Note: The core/thread indices as seen by the L3 cache do not match what the
// CPU reports, especially when cores are disabled (4/6-core processors).
uint8_t pmc_cpu_to_thread_mask(int cpu);

// Prints the current configuration of all L3 counters.
void pmc_print_l3_cfg(int cpu);

#endif
