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
		unsigned EventSel:8;
		unsigned UnitMask:8;
		unsigned _reserved1:6;
		bool Enable:1;
		unsigned _reserved2:25;
		unsigned SliceMask:4;
		unsigned _reserved3:4;
		unsigned ThreadMask:8;
	};
	uint64_t value;
};
//_Static_assert(sizeof(union pmc_l3_event) == 8, "pmc_l3_event has correct size");

#define PMC_CURRENT_CPU -1

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
uint8_t pmc_cpu_to_thread_mask(int cpu);

#endif
