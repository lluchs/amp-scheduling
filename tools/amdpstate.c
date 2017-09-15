#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "msrtools.h"
#include "amdccx.h"

static const uint32_t
	PStateCurLim   = 0xc0010061,
	PStateCtl      = 0xc0010062,
	PStateStat     = 0xc0010063,
	PStateDef      = 0xc0010064,
	PStateDef_last = 0xc001006c;

// Components of the PStateCurLim register.
union PStateCurLim {
	struct {
		unsigned CurPstateLimit:3;
		unsigned _reserved1:1;
		unsigned PstateMaxVal:3;
	};
	uint64_t value;
};

// Components of the PStateDef register.
union PStateDef {
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
static int CoreCOF(union PStateDef pstate) {
	return pstate.CpuDfsId > 0 ? 200 * pstate.CpuFid / pstate.CpuDfsId : -1;
}

// Core voltage in Volt for a given PStateDef.
// TODO: Formular is from some overclocking forums, any better references?
static double CoreVoltage(union PStateDef pstate) {
	return 1.55 - 0.00625 * (double) pstate.CpuVid;
}

static void print_pstate_info(int cpu) {
	union PStateCurLim cur_lim = {.value = rdmsr_on_cpu(PStateCurLim, cpu)};
	printf("CPU %2d: CCX ID = %u\n", cpu, apicid_on_cpu(cpu).CCXID);
	printf("CPU %2d: PStateCurLim = min %u max %u\n", cpu, cur_lim.CurPstateLimit, cur_lim.PstateMaxVal);
	printf("CPU %2d: PStateCtl = %"PRIu64"\n", cpu, rdmsr_on_cpu(PStateCtl, cpu) & 0x7);
	printf("CPU %2d: PStateStat = %"PRIu64"\n", cpu, rdmsr_on_cpu(PStateStat, cpu) & 0x7);
	union PStateDef pstate;
	for (uint32_t reg = PStateDef; reg < PStateDef_last; reg++) {
		pstate.value = rdmsr_on_cpu(reg, cpu);
		printf("CPU %2d: PStateDef[%"PRIu32"] = PStateEn %u IddDiv %u IddValue %u CpuVid %u CpuDfsId 0x%u CpuFid %u\n",
				cpu, reg - PStateDef,
				pstate.PStateEn, pstate.IddDiv, pstate.IddValue, pstate.CpuVid, pstate.CpuDfsId, pstate.CpuFid);
		printf("                    => CoreCOF %d MHz CoreVoltage %f V\n", CoreCOF(pstate), CoreVoltage(pstate));
	}
}

static void usage(char *argv0) {
	fprintf(stderr, "Usage: %s [-c cpu] COMMAND\n", argv0);
	fprintf(stderr, "where COMMAND := { status | change }\n");
	exit(1);
}

int main(int argc, char *argv[]) {
	int cpu = 0;

	int opt;
	while ((opt = getopt(argc, argv, "c:")) != -1) {
		switch (opt) {
		case 'c':
			cpu = atoi(optarg);
			break;
		default:
			usage(argv[0]);
		}
	}

	if (optind >= argc) {
		usage(argv[0]);
	}

	char *cmd = argv[optind];
	if (strcmp(cmd, "status") == 0) {
		print_pstate_info(cpu);
	} else if (strcmp(cmd, "change") == 0) {
		if (optind+1 >= argc) {
change_usage:
			fprintf(stderr, "Usage: %s change STATE\nwhere STATE between 0 and 7\n\n", argv[0]);
			usage(argv[0]);
		}
		int state = atoi(argv[optind+1]);
		if (state < 0 || state > 7) goto change_usage;
		wrmsr_on_cpu(PStateCtl, cpu, (uint64_t) state);
	} else {
		usage(argv[0]);
	}
}
