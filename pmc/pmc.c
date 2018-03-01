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

/* Library for reading L3 cache counters on Ryzen. */

#include "pmc.h"
#include "../tools/msrtools.h"
#include "../tools/amdccx.h"

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/syscall.h>

static const uint32_t
	ChL3PmcCfg = 0xc0010230, // + ctr*2
	ChL3Pmc    = 0xc0010231, // + ctr*2
	IRPerfRO   = 0xc00000e9;

static inline int getcpu() {
	unsigned cpu;
	syscall(SYS_getcpu, &cpu, NULL, NULL);
	return cpu;
}

uint64_t pmc_read_ir(int cpu) {
	if (cpu == PMC_CURRENT_CPU) cpu = getcpu();
	return rdmsr_on_cpu(IRPerfRO, cpu);
}

void pmc_select_l3_event(int cpu, uint8_t ctr, union pmc_l3_event event) {
	if (cpu == PMC_CURRENT_CPU) cpu = getcpu();
	wrmsr_on_cpu(ChL3PmcCfg + ctr*2, cpu, event.value);
}

void pmc_write_l3_counter(int cpu, uint8_t ctr, uint64_t value) {
	if (cpu == PMC_CURRENT_CPU) cpu = getcpu();
	wrmsr_on_cpu(ChL3Pmc + ctr*2, cpu == -1 ? getcpu() : cpu, value);
}

#ifndef USE_PMC
uint64_t pmc_read_l3_counter(uint8_t ctr) {
	return rdmsr_on_cpu(ChL3Pmc + ctr*2, getcpu());
}
#endif

uint8_t pmc_cpu_to_thread_mask(int cpu) {
	union ApicId aid = apicid_on_cpu(cpu);
	return 1 << aid.CoreAndThreadId;
}

void pmc_print_l3_cfg(int cpu) {
	if (cpu == PMC_CURRENT_CPU) cpu = getcpu();
	union pmc_l3_event event;
	for (int ctr = 0; ctr < 6; ctr++) {
		event.value = rdmsr_on_cpu(ChL3PmcCfg + ctr*2, cpu);
		if (event.Enable)
			printf("Counter %d: EventSel %x UnitMask %x SliceMask %x ThreadMask %x\n", ctr, event.EventSel, event.UnitMask, event.SliceMask, event.ThreadMask);
	}
}
