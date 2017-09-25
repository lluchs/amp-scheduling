#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include "ultmigration.h"
#include "random.h"

#define WORK 10000
#define BUFSIZE (size_t) (500 << 20)
#define PATTERN_LENGTH 100000

static int lotsofwork() {
	unsigned int foo1 = 2, foo2 = 3, foo3 = 4, foo4 = 5;
	for (int i = 0; i < 100000; i++) {
		foo1 = foo1 * foo1; foo2 = foo2 * foo2; foo3 = foo3 * foo3; foo4 = foo4 * foo4;
		foo1 = foo1 * foo1; foo2 = foo2 * foo2; foo3 = foo3 * foo3; foo4 = foo4 * foo4;
	}
	return (foo1 + foo2 + foo3 + foo4) % 3;
}

static char *buf;
static size_t *access_pattern;

static int littlework() {
	for (int i = 0; i < PATTERN_LENGTH; i++) {
		buf[access_pattern[i]] = i;
	}
	return buf[access_pattern[0]];
}

static void init_littlework() {
	random_init();
	buf = malloc(BUFSIZE);
	access_pattern = malloc(PATTERN_LENGTH * sizeof(*access_pattern));
	for (int i = 0; i < PATTERN_LENGTH; i++) {
		access_pattern[i] = random_next() % BUFSIZE;
	}
}

int main() {
	init_littlework();

	ult_register_klt();
	ult_migrate(ULT_FAST);
	fprintf(stderr, "fast CPU = %d\n", sched_getcpu());
	ult_migrate(ULT_SLOW);
	fprintf(stderr, "slow CPU = %d\n", sched_getcpu());
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
