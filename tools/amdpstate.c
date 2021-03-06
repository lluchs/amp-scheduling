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

/* Tool for reading and changing P-state configuration on Ryzen. */

#include <math.h>
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
	PStateDef_last = 0xc001006c,
	MPerf          = 0x000000e7,
	APerf          = 0x000000e8,
	MPerfReadOnly  = 0xc00000e7,
	APerfReadOnly  = 0xc00000e8,
	RAPLPowerUnit  = 0xc0010299,
	CoreEnergyStat = 0xc001029a,
	PkgEnergyStat  = 0xc001029b;

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

#define ALL_CPUS -1
#define cpu_loop(i, sel) for ((i) = (sel) == ALL_CPUS ? 0 : (sel); ((sel) == ALL_CPUS && (i) < sysconf(_SC_NPROCESSORS_ONLN)) || (i) == (sel); (i)++)

// See 2.1.4 Effective Frequency
static void print_frequency_info(int which_cpu, int duration) {
	uint64_t mperf, aperf, mperf_ro, aperf_ro;
	int freq, freq_ro, p0freq;
	int cpu;
	if (duration > 0) {
		// Reset APerf and MPerf and wait.
		cpu_loop(cpu, which_cpu) {
			wrmsr_on_cpu(MPerf, cpu, 0);
			wrmsr_on_cpu(APerf, cpu, 0);
		}
		usleep(duration * 1000);
	}
	cpu_loop(cpu, which_cpu) {
		mperf = rdmsr_on_cpu(MPerf, cpu);
		aperf = rdmsr_on_cpu(APerf, cpu);
		mperf_ro = rdmsr_on_cpu(MPerfReadOnly, cpu);
		aperf_ro = rdmsr_on_cpu(APerfReadOnly, cpu);
		p0freq = CoreCOF((union PStateDef) {.value = rdmsr_on_cpu(PStateDef, cpu)});
		freq = p0freq * ((long double) aperf / mperf);
		freq_ro = p0freq * ((long double) aperf_ro / mperf_ro);
		printf("CPU %2d: P0 frequency = %d MHz\n", cpu, p0freq);
		printf("CPU %2d: effective frequency (ro) = %d MHz\n", cpu, freq_ro);
		if (duration > 0)
			printf("CPU %2d: effective frequency      = %d MHz\n", cpu, freq);
	}
}

// From CodeXL/Components/PowerProfiling/Backend/AMDTPowerProfileAPI/src/PowerProfile{Translate.cpp,Helper.h}
#define ZEPPELIN_ENERGY_VAL 0x10
static uint32_t rapl_get_energy_unit()
{
	uint64_t data = rdmsr_on_cpu(RAPLPowerUnit, 0);
    return (uint8_t)((data & 0x1F00) >> 8) ? (uint8_t)((data & 0x1F00) >> 8) : ZEPPELIN_ENERGY_VAL;
}

static void print_rapl_info(int which_cpu, uint32_t duration) {
	uint32_t pkg_energy, core_energy;
	uint32_t last_pkg_energy = 0, last_core_energy[128] = {0};
	double unit = pow(2, rapl_get_energy_unit());
	int cpu, core;
	for (;;) {
		pkg_energy = rdmsr_on_cpu(PkgEnergyStat, which_cpu >= 0 ? which_cpu : 0);
		if (last_pkg_energy) {
			printf("Package Power = %f W\n", (pkg_energy - last_pkg_energy) / unit * 1000 / duration);
		}
		last_pkg_energy = pkg_energy;
		cpu_loop(cpu, ALL_CPUS) {
			if (cpu % 2) continue;
			core = cpu / 2;
			core_energy = rdmsr_on_cpu(CoreEnergyStat, cpu);
			if (last_core_energy[core])
				printf("Core %d: Core Power = %5f W\n", core, (core_energy - last_core_energy[core]) / unit * 1000 / duration);
			last_core_energy[core] = core_energy;
		}
		usleep(duration * 1000);
	}
}

static void usage(char *argv0) {
	fprintf(stderr, "Usage: %s [-c cpu] COMMAND\n", argv0);
	fprintf(stderr, "where COMMAND := { status | frequency | change | def | rapl }\n");
	exit(1);
}

static int cpu = 0;

static void def_pstate(int argc, char *argv[]) {
	if (argc < 3) {
def_pstate_usage:
		fprintf(stderr, "Usage: ... %s STATE COMPONENT=VALUE...\n", argv[0]);
		fprintf(stderr, "where STATE between 0 and 7\n");
		fprintf(stderr, "      COMPONENT := { PstateEn | IddDiv | IddValue | CpuVid | CpuDfsId | CpuFid }\n\n");
		usage("...");
	}
	int state = atoi(argv[1]);
	if (state < 0 || state > 7) {
		fprintf(stderr, "Invalid state %d\n", state);
		goto def_pstate_usage;
	}
	uint32_t reg = PStateDef + state;
	union PStateDef pstate = {.value = rdmsr_on_cpu(reg, cpu)};
	unsigned long arg;
	for (int i = 2; i < argc; i++) {
		char *kv = argv[i];
#define IS(name) (strncmp(name "=", kv, strlen(name "=")) == 0 && (kv += strlen(name "=")))
#define CHECK_ARG(min, max) do { arg = strtoul(kv, NULL, 0); if (arg < (min) || arg > (max)) { fprintf(stderr, "Invalid value %lu, must be between %d and %d\n", arg, min, max); goto def_pstate_usage; } } while (0)
		if (IS("PstateEn")) {
			CHECK_ARG(0, 1);
			pstate.PStateEn = arg;
		} else if (IS("IddDiv")) {
			CHECK_ARG(0, 3);
			pstate.IddDiv = arg;
		} else if (IS("IddValue")) {
			CHECK_ARG(0, 0xff);
			pstate.IddValue = arg;
		} else if (IS("CpuVid")) {
			CHECK_ARG(0, 0xff);
			pstate.CpuVid = arg;
		} else if (IS("CpuDfsId")) {
			CHECK_ARG(0, 0x3f);
			pstate.CpuDfsId = arg;
		} else if (IS("CpuFid")) {
			CHECK_ARG(0, 0xff);
			pstate.CpuFid = arg;
		} else {
			fprintf(stderr, "Invalid component %s\n", kv);
			goto def_pstate_usage;
		}
#undef CHECK_ARG
#undef IS
	}
	// The PPR requires P-state definitions to be identical on all cores of a
	// coherence fabric, so just set it for all CPUs here.
	wrmsr_on_all_cpus(reg, pstate.value);
}

int main(int argc, char *argv[]) {
	int opt;
	while ((opt = getopt(argc, argv, "+c:")) != -1) {
		switch (opt) {
		case 'c':
			if (strcmp(optarg, "all") == 0)
				cpu = ALL_CPUS;
			else
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
	} else if (strcmp(cmd, "frequency") == 0) {
		print_frequency_info(cpu, optind+1 < argc ? atoi(argv[optind+1]) : 0);
	} else if (strcmp(cmd, "change") == 0) {
		if (optind+1 >= argc) {
change_usage:
			fprintf(stderr, "Usage: %s change STATE\nwhere STATE between 0 and 7\n\n", argv[0]);
			usage(argv[0]);
		}
		int state = atoi(argv[optind+1]);
		if (state < 0 || state > 7) goto change_usage;
		wrmsr_on_cpu(PStateCtl, cpu, (uint64_t) state);
	} else if (strcmp(cmd, "def") == 0) {
		def_pstate(argc - optind, argv + optind);
	} else if (strcmp(cmd, "rapl") == 0) {
		uint32_t duration = optind+1 <= argc ? atoi(argv[optind+1]) : 1000;
		print_rapl_info(cpu, duration);
	} else {
		usage(argv[0]);
	}
}
