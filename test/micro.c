#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include "ultmigration.h"
#include "random.h"

// Overall iterations.
#define WORK 10000
// Number of CPU work iterations.
#define CPU_ITERATIONS 400000

// Size of memory buffer used for indirect_access and pointer_chase
#define BUFSIZE (size_t) (100 << 20)

// indirect_access: Length of access pattern.
#define PATTERN_LENGTH 100000
// pointer_chasing: Number of steps per iteration.
#define POINTER_CHASE_STEPS 7000

static int lotsofwork() {
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

typedef int (*Fn)();

static void usage(char **argv) {
	fprintf(stderr, "Usage: %s MEMORY_BENCH\n", argv[0]);
	fprintf(stderr, "where MEMORY_BENCH := { indirect_access | pointer_chasing }\n");
	exit(1);
}

int main(int argc, char **argv) {
	random_init();

	if (argc != 2) usage(argv);

	Fn littlework;
	char *memory_bench = argv[1];
	if (strcmp("indirect_access", memory_bench) == 0) {
		init_indirect_access();
		littlework = indirect_access;
	} else if (strcmp("pointer_chasing", memory_bench) == 0) {
		init_pointer_chasing();
		littlework = pointer_chasing;
	} else {
		fprintf(stderr, "Unknown MEMORY_BENCH %s\n\n", memory_bench);
		usage(argv);
	}

	ult_register_klt();
	ult_migrate(ULT_FAST);
	fprintf(stderr, "fast CPU = %d\n", sched_getcpu());
	ult_migrate(ULT_SLOW);
	fprintf(stderr, "slow CPU = %d\n", sched_getcpu());
	fprintf(stderr, "MEMORY_BENCH = %s\n", memory_bench);
	fprintf(stderr, "buffer size = %lu MiB\n", BUFSIZE >> 20);

	int result = 0;
	for (int i = 0; i < WORK; i++) {
		ult_migrate(ULT_FAST);
		result += lotsofwork();
		ult_migrate(ULT_SLOW);
		result += littlework();
	}
	printf("done %d\n", result);
	ult_unregister_klt();
}
