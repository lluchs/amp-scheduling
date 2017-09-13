#include <stdio.h>
#include "msrtools.h"

static const uint32_t
	PStateCurLim   = 0xc0010061,
	PStateCtl      = 0xc0010062,
	PStateStat     = 0xc0010063,
	PStateDef      = 0xc0010064,
	PStateDef_last = 0xc001006c;

int main() {
	int cpu = 0;
	uint64_t cur_lim = rdmsr_on_cpu(PStateCurLim, cpu);
	printf("CPU %d: PStateCurLim = max %"PRIu64" min %"PRIu64"\n", cpu, (cur_lim & 0x70) >> 4, cur_lim & 0x7);
	printf("CPU %d: PStateCtl = %"PRIu64"\n", cpu, rdmsr_on_cpu(PStateCtl, cpu) & 0x7);
	printf("CPU %d: PStateStat = %"PRIu64"\n", cpu, rdmsr_on_cpu(PStateStat, cpu) & 0x7);
	for (uint32_t reg = PStateDef; reg < PStateDef_last; reg++) {
		uint64_t def = rdmsr_on_cpu(reg, cpu);
		printf("CPU %d: PStateDef[%"PRIu32"] = PStateEn %"PRIu64" IddDiv %"PRIu64" IddValue %"PRIu64" CpuVid %"PRIu64" CpuDfsId 0x%"PRIx64" CpuFid %"PRIu64"\n",
				cpu, reg - PStateDef,
				def >> 63, /* PstateEn */
				def >> 30 & 0x3, def >> 22 & 0xff, /* IddDiv, IddValue */
				def >> 14 & 0xff, /* CpuVid */
				def >> 8 & 0x3f, /* CpuDfsId */
				def & 0xff); /* CpuFid */
	}
}
