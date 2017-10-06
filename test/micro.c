#define _GNU_SOURCE
#include <assert.h>
#include <cpuid.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include "ultmigration.h"
#include "random.h"
#include "pmc/pmc.h"
#include "tools/msrtools.h"

// Multiplier for overall iterations.
// Increasing this number makes core switches more frequent while keeping the
// overall amount of work constant.
#define OUTER_WORK_MULT 1

// Overall iterations.
#define WORK (10000 * OUTER_WORK_MULT)
// Number of CPU work iterations.
#define CPU_ITERATIONS (400000 / OUTER_WORK_MULT)

// Size of memory buffer used for indirect_access and pointer_chase
#define BUFSIZE (size_t) (100 << 20)

// indirect_access: Length of access pattern.
#define PATTERN_LENGTH (100000 / OUTER_WORK_MULT)
// pointer_chasing: Number of steps per iteration.
#define POINTER_CHASE_STEPS (7000 / OUTER_WORK_MULT)

static int cpubench() {
	unsigned int foo1 = 2, foo2 = 3, foo3 = 4, foo4 = 5;
	float bar1 = 2, bar2 = 3, bar3 = 4, bar4 = 5;
	for (int i = 0; i < CPU_ITERATIONS; i++) {
		foo1 = foo1 * foo1; foo2 = foo2 + foo2; foo3 = foo3 * foo3; foo4 = foo4 + foo4;
		bar1 = bar1 * bar1; bar2 = bar2 + bar2; bar3 = bar3 * bar3; bar4 = bar4 + bar4;
		foo1 = foo1 * foo1; foo2 = foo2 + foo2; foo3 = foo3 * foo3; foo4 = foo4 + foo4;
		bar1 = bar1 * bar1; bar2 = bar2 + bar2; bar3 = bar3 * bar3; bar4 = bar4 + bar4;
	}
	return (foo1 + foo2 + foo3 + foo4 + (unsigned) (bar1 + bar2 + bar3 + bar4)) % 3;
}

static char *indirect_access_buf;
static size_t *access_pattern;

static int indirect_access() {
	for (int i = 0; i < PATTERN_LENGTH; i++) {
		indirect_access_buf[access_pattern[i]] = i;
	}
	return indirect_access_buf[access_pattern[0]];
}

static void init_indirect_access() {
	indirect_access_buf = malloc(BUFSIZE);
	access_pattern = malloc(PATTERN_LENGTH * sizeof(*access_pattern));
	for (int i = 0; i < PATTERN_LENGTH; i++) {
		access_pattern[i] = random_next() % BUFSIZE;
	}
}

#define CH_BUFLEN (BUFSIZE / sizeof(void*))
static void **pointer_chasing_buf, **pointer_chasing_ptr;

static int pointer_chasing() {
	int r = 0;
	for (int i = 0; i < POINTER_CHASE_STEPS; i++) {
		pointer_chasing_ptr = *pointer_chasing_ptr;
		r += (intptr_t) pointer_chasing_ptr;
	}
	return r;
}

static void init_pointer_chasing() {
	pointer_chasing_buf = malloc(BUFSIZE);
	// Initialize sequential list of pointers.
	void **ptr;
	for (ptr = pointer_chasing_buf; ptr < pointer_chasing_buf + CH_BUFLEN; ptr++) {
		*ptr = ptr+1;
	}
	*(ptr-1) = pointer_chasing_buf;
	// Shuffle the pointers.
	size_t m = CH_BUFLEN, i;
	void *tmp;
	while (m) {
		i = random_next() % m--;
		tmp = pointer_chasing_buf[m];
		pointer_chasing_buf[m] = pointer_chasing_buf[i];
		pointer_chasing_buf[i] = tmp;
	}
	pointer_chasing_ptr = pointer_chasing_buf;
}

static int noop() { return 0; }

// L3 counter indices
enum {
	L3Ctr_Fast,
	L3Ctr_Slow,
};

static int getcore() {
	unsigned a, b = 0, c, d;
	__get_cpuid(0x8000001e, &a, &b, &c, &d);
	return b & 0xff;
}

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

typedef int (*Fn)();

static void usage(char **argv) {
	fprintf(stderr, "Usage: %s MEMORY_BENCH CPU_BENCH\n", argv[0]);
	fprintf(stderr, "where MEMORY_BENCH := { none | indirect_access | pointer_chasing }\n");
	fprintf(stderr, "where CPU_BENCH := { none | some }\n");
	exit(1);
}

int main(int argc, char **argv) {
	random_init();
	init_dev_msr();

	if (argc != 3) usage(argv);

	Fn littlework, lotsofwork;
	char *memory_bench = argv[1], *cpu_bench = argv[2];
	if (strcmp("none", memory_bench) == 0) {
		littlework = noop;
	} else if (strcmp("indirect_access", memory_bench) == 0) {
		init_indirect_access();
		littlework = indirect_access;
	} else if (strcmp("pointer_chasing", memory_bench) == 0) {
		init_pointer_chasing();
		littlework = pointer_chasing;
	} else {
		fprintf(stderr, "Unknown MEMORY_BENCH %s\n\n", memory_bench);
		usage(argv);
	}
	if (strcmp("none", cpu_bench) == 0) {
		lotsofwork = noop;
	} else if (strcmp("some", cpu_bench) == 0) {
		lotsofwork = cpubench;
	} else {
		fprintf(stderr, "Unknown CPU_BENCH %s\n\n", cpu_bench);
		usage(argv);
	}

	ult_register_klt();
	ult_migrate(ULT_FAST);
	int fast_cpu = sched_getcpu();
	fprintf(stderr, "fast CPU = %d, core = %d\n", fast_cpu, getcore());
	ult_migrate(ULT_SLOW);
	int slow_cpu = sched_getcpu();
	fprintf(stderr, "slow CPU = %d, core = %d\n", slow_cpu, getcore());
	fprintf(stderr, "MEMORY_BENCH = %s\n", memory_bench);
	fprintf(stderr, "CPU_BENCH = %s\n", cpu_bench);
	fprintf(stderr, "buffer size = %lu MiB\n", BUFSIZE >> 20);

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
	ult_unregister_klt();
}
