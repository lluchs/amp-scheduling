#define _GNU_SOURCE
#include <assert.h>
#include <cpuid.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include "ultmigration.h"
#include "random.h"

#include "micro.h"

static int do_cpubench(int N) {
	unsigned int foo1 = 2, foo2 = 3, foo3 = 4, foo4 = 5;
	float bar1 = 2, bar2 = 3, bar3 = 4, bar4 = 5;
	for (int i = 0; i < N; i++) {
		foo1 = foo1 * foo1; foo2 = foo2 + foo2; foo3 = foo3 * foo3; foo4 = foo4 + foo4;
		bar1 = bar1 * bar1; bar2 = bar2 + bar2; bar3 = bar3 * bar3; bar4 = bar4 + bar4;
		foo1 = foo1 * foo1; foo2 = foo2 + foo2; foo3 = foo3 * foo3; foo4 = foo4 + foo4;
		bar1 = bar1 * bar1; bar2 = bar2 + bar2; bar3 = bar3 * bar3; bar4 = bar4 + bar4;
	}
	return (foo1 + foo2 + foo3 + foo4 + (unsigned) (bar1 + bar2 + bar3 + bar4)) % 3;
}

static int cpubench() {
	return do_cpubench(CPU_ITERATIONS);
}

static char *indirect_access_buf;
static size_t *access_pattern;

static int do_indirect_access(int N) {
	static unsigned p = 0;
	for (int i = 0; i < N; i++) {
		indirect_access_buf[access_pattern[p++ % PATTERN_LENGTH]] = i;
	}
	return indirect_access_buf[access_pattern[0]];
}

static int indirect_access() {
	return do_indirect_access(PATTERN_LENGTH);
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

static int do_pointer_chasing(int N) {
	int r = 0;
	for (int i = 0; i < N; i++) {
		pointer_chasing_ptr = *pointer_chasing_ptr;
		r += (intptr_t) pointer_chasing_ptr;
	}
	return r;
}

static int pointer_chasing() {
	return do_pointer_chasing(POINTER_CHASE_STEPS);
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

typedef int (*FnN)(int); // do_ functions
static FnN mixed_little_fn, mixed_lots_fn;
static int mixed_little_steps, mixed_lots_steps;
// 1 => do only little/lots work.
static float mixed_little_ratio, mixed_lots_ratio;

// ratio = 1 => only little, ratio = 0 => only lots
static int do_mixed(float ratio) {
	int result = 0;
	int little_steps = mixed_little_steps * ratio;
	int lots_steps = mixed_lots_steps * (1-ratio);
	int N = little_steps + lots_steps;
	int min_steps = little_steps < lots_steps ? little_steps : lots_steps;
	little_steps /= min_steps;
	lots_steps /= min_steps;
	for (int i = 0; i < N; i += little_steps + lots_steps) {
		result += mixed_little_fn(little_steps);
		result += mixed_lots_fn(lots_steps);
	}
	return result;
}

static int mixed_lots() {
	return do_mixed(1-mixed_lots_ratio);
}

static int mixed_little() {
	return do_mixed(mixed_little_ratio);
}

static int noop() { return 0; }

static int getcore() {
	unsigned a, b = 0, c, d;
	__get_cpuid(0x8000001e, &a, &b, &c, &d);
	return b & 0xff;
}

static void usage(char **argv) {
	fprintf(stderr, "Usage: %s [options] MEMORY_BENCH CPU_BENCH\n", argv[0]);
	fprintf(stderr, "where MEMORY_BENCH := { none | indirect_access | pointer_chasing }\n");
	fprintf(stderr, "where CPU_BENCH := { none | some }\n");
	fprintf(stderr, "\nOptions:\n");
	fprintf(stderr, "\t--memory-ratio=<n ϵ [0, 1]>: interleave MEMORY_BENCH with CPU_BENCH\n");
	fprintf(stderr, "\t--cpu-ratio=<n ϵ [0, 1]>:    interleave CPU_BENCH with MEMORY_BENCH\n");
	fprintf(stderr, "\t--only-cpu:                  only run CPU_BENCH\n");
	fprintf(stderr, "\t--only-memory:               only run MEMORY_BENCH\n");
	exit(1);
}

// Main benchmark function, marked as weak to allow overriding in variants.
void __attribute__((weak)) run_benchmark(Fn littlework, Fn lotsofwork) {
	int result = 0;
	for (int i = 0; i < WORK; i++) {
		ult_migrate(ULT_FAST);
		result += lotsofwork();

		ult_migrate(ULT_SLOW);
		result += littlework();
	}
	printf("done %d\n", result);
}

// Global symbols to allow access from variants.
int slow_cpu, fast_cpu;

int main(int argc, char **argv) {
	random_init();

	// Option parsing.
	bool have_mixed_lots = false, have_mixed_little = false;
	Fn littlework = NULL, lotsofwork = NULL;
	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"cpu-ratio",     required_argument,  0,  'c'},
			{"memory-ratio",  required_argument,  0,  'm'},
			{"only-cpu",      no_argument,        0,  'C'},
			{"only-memory",   no_argument,        0,  'M'},
			{0,               0,                  0,  0}
		};
		int c = getopt_long(argc, argv, "h", long_options, &option_index);
		if (c == -1) break;
		switch (c) {
		case 'c':
			mixed_lots_ratio = strtof(optarg, NULL);
			have_mixed_lots = true;
			break;
		case 'm':
			mixed_little_ratio = strtof(optarg, NULL);
			have_mixed_little = true;
			break;
		case 'C':
			littlework = noop;
			break;
		case 'M':
			lotsofwork = noop;
			break;
		default:
			usage(argv);
		}
	}
	if (argc-optind != 2) usage(argv);

	char *memory_bench = argv[optind], *cpu_bench = argv[optind+1];
	if (strcmp("none", memory_bench) == 0) {
		littlework = mixed_little_fn = noop;
	} else if (strcmp("indirect_access", memory_bench) == 0) {
		init_indirect_access();
		if (!littlework) littlework = indirect_access;
		mixed_little_fn = do_indirect_access;
		mixed_little_steps = PATTERN_LENGTH;
	} else if (strcmp("pointer_chasing", memory_bench) == 0) {
		init_pointer_chasing();
		if (!littlework) littlework = pointer_chasing;
		mixed_little_fn = do_pointer_chasing;
		mixed_little_steps = POINTER_CHASE_STEPS;
	} else {
		fprintf(stderr, "Unknown MEMORY_BENCH %s\n\n", memory_bench);
		usage(argv);
	}
	if (strcmp("none", cpu_bench) == 0) {
		lotsofwork = mixed_lots_fn = noop;
	} else if (strcmp("some", cpu_bench) == 0) {
		if (!lotsofwork) lotsofwork = cpubench;
		mixed_lots_fn = do_cpubench;
		mixed_lots_steps = CPU_ITERATIONS;
	} else {
		fprintf(stderr, "Unknown CPU_BENCH %s\n\n", cpu_bench);
		usage(argv);
	}
	if (have_mixed_little) littlework = mixed_little;
	if (have_mixed_lots) lotsofwork = mixed_lots;

	ult_register_klt();
	ult_migrate(ULT_FAST);
	fast_cpu = sched_getcpu();
	fprintf(stderr, "fast CPU = %d, core = %d\n", fast_cpu, getcore());
	ult_migrate(ULT_SLOW);
	slow_cpu = sched_getcpu();
	fprintf(stderr, "slow CPU = %d, core = %d\n", slow_cpu, getcore());
	fprintf(stderr, "MEMORY_BENCH = %s", memory_bench);
	if (have_mixed_little)
		fprintf(stderr, " @ %f", mixed_little_ratio);
	if (lotsofwork == noop)
		fputs(" (only)", stderr);
	putc('\n', stderr);
	fprintf(stderr, "CPU_BENCH = %s", cpu_bench);
	if (have_mixed_lots)
		fprintf(stderr, " @ %f", mixed_lots_ratio);
	if (littlework == noop)
		fputs(" (only)", stderr);
	putc('\n', stderr);
	fprintf(stderr, "buffer size = %lu MiB\n", BUFSIZE >> 20);

	run_benchmark(littlework, lotsofwork);

	ult_unregister_klt();
}
