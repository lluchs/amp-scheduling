#define _GNU_SOURCE
#include "ultmigration.h"
#include <assert.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

static int registered = 0;
static int pstates[8]; // Ryzen supports max. 8 P-states. The array maps pstate number to cpufreq frequency.
static int pstate_max = 0; // Maximum valid index in pstates
static int pstate_fast, pstate_slow; // P-state indices for ULT_FAST/ULT_SLOW
static FILE *setspeed; // scaling_setspeed file

void ult_register_klt(void) {
	assert(!registered && "can't register more than one thread");
	registered = 1;

	// Pin the thread to the currently active CPU.
	int cpu = sched_getcpu();
	cpu_set_t cpus;
	CPU_ZERO(&cpus);
	CPU_SET(cpu, &cpus);
	sched_setaffinity(0, sizeof(cpus), &cpus);

	// Open the thread's P-state control file.
	char filename[100];
	sprintf(filename, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_available_frequencies", cpu);
	FILE *f = fopen(filename, "r"); assert(f && "couldn't open scaling_available_frequencies");
	pstate_max = fscanf(f, "%d %d %d %d %d %d %d %d", &pstates[0], &pstates[1], &pstates[2], &pstates[3], &pstates[4], &pstates[5], &pstates[6], &pstates[7]) - 1;
	assert(pstate_max > 0 && "didn't read any P-states from scaling_available_frequencies");
	fclose(f);

	sprintf(filename, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_setspeed", cpu);
	setspeed = fopen(filename, "w"); assert(setspeed && "couldn't open scaling_setspeed");

	// Read fast/slow P-state indices from environment.
	char *slow_idx = getenv("SLOW_IDX");
	char *fast_idx = getenv("FAST_IDX");
	assert(slow_idx != NULL && fast_idx != NULL && "SLOW_IDX or FAST_IDX environment variable not set");
	pstate_slow = atoi(slow_idx);
	pstate_fast = atoi(fast_idx);
	assert(pstate_slow >= 0 && pstate_slow <= pstate_max && pstate_fast >= 0 && pstate_fast <= pstate_max && "invalid index");
}

void ult_unregister_klt(void) {
	assert(registered && "not previously registered");
	registered = 0;
	fclose(setspeed);
}

void ult_migrate(enum ult_thread_type type) {
	int frequency;
	switch (type) {
	case ULT_FAST: frequency = pstates[pstate_fast]; break;
	case ULT_SLOW: frequency = pstates[pstate_slow]; break;
	default: assert(!"invalid type");
	}
	fprintf(setspeed, "%d\n", frequency);
	fflush(setspeed);
}

int ult_registered(void) { return registered; }

