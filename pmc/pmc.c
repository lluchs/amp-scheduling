#include "pmc.h"
#include "../tools/msrtools.h"
#include "../tools/amdccx.h"

#include <assert.h>
#include <unistd.h>
#include <sys/syscall.h>

static const uint32_t
	ChL3PmcCfg = 0xc0010230, // + ctr*2
	ChL3Pmc    = 0xc0010231; // + ctr*2

static inline int getcpu() {
	unsigned cpu;
	syscall(SYS_getcpu, &cpu, NULL, NULL);
	return cpu;
}

void pmc_select_l3_event(int cpu, uint8_t ctr, union pmc_l3_event event) {
	if (cpu == PMC_CURRENT_CPU) cpu = getcpu();
	uint64_t msr = (uint64_t) event.EventSel
		| ((uint64_t) event.UnitMask   <<  8)
		| ((uint64_t) event.Enable     << 22)
		| ((uint64_t) event.SliceMask  << 48)
		| ((uint64_t) event.ThreadMask << 56);
	// TODO: This is supposed to be the same...
	//printf("event = %"PRIx64"\n", event.value);
	//printf("msr   = %"PRIx64"\n", msr);
	wrmsr_on_cpu(ChL3PmcCfg + ctr*2, cpu, msr);
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
