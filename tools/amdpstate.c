#include <stdio.h>
#include "msrtools.h"

static const uint32_t
	PStateCurLim   = 0xc0010061,
	PStateCtl      = 0xc0010062,
	PStateStat     = 0xc0010063,
	PStateDef      = 0xc0010064,
	PStateDef_last = 0xc001006c;

// Components of the PStateDef register.
union PState {
	struct {
		unsigned CpuFid:8;
		unsigned CpuDfsId:6;
		unsigned CpuVid:8;
		unsigned IddValue:8;
		unsigned IddDiv:2;
		unsigned _reserved:31;
		unsigned PStateEn:1;
	};
	uint64_t value;
};

// Core current operating frequency in MHz for a given PStateDef.
// Defined in AMD PPR Table 14 (Definitions).
static int CoreCOF(union PState pstate) {
	return pstate.CpuDfsId > 0 ? 200 * pstate.CpuFid / pstate.CpuDfsId : -1;
}

// Core voltage in Volt for a given PStateDef.
// TODO: Formular is from some overclocking forums, any better references?
static double CoreVoltage(union PState pstate) {
	return 1.55 - 0.00625 * (double) pstate.CpuVid;
}

static void print_pstate_info(int cpu) {
	uint64_t cur_lim = rdmsr_on_cpu(PStateCurLim, cpu);
	printf("CPU %2d: PStateCurLim = max %"PRIu64" min %"PRIu64"\n", cpu, (cur_lim & 0x70) >> 4, cur_lim & 0x7);
	printf("CPU %2d: PStateCtl = %"PRIu64"\n", cpu, rdmsr_on_cpu(PStateCtl, cpu) & 0x7);
	printf("CPU %2d: PStateStat = %"PRIu64"\n", cpu, rdmsr_on_cpu(PStateStat, cpu) & 0x7);
	union PState pstate;
	for (uint32_t reg = PStateDef; reg < PStateDef_last; reg++) {
		pstate.value = rdmsr_on_cpu(reg, cpu);
		printf("CPU %2d: PStateDef[%"PRIu32"] = PStateEn %u IddDiv %u IddValue %u CpuVid %u CpuDfsId 0x%u CpuFid %u\n",
				cpu, reg - PStateDef,
				pstate.PStateEn, pstate.IddDiv, pstate.IddValue, pstate.CpuVid, pstate.CpuDfsId, pstate.CpuFid);
		printf("                    => CoreCOF %d MHz CoreVoltage %f V\n", CoreCOF(pstate), CoreVoltage(pstate));
	}
}

int main() {
	int cpu = 0;
	print_pstate_info(cpu);
}
