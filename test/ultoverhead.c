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

/* Benchmark for measuring performance overhead from ultmigration. */

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <string.h>
#include "ultmigration.h"

#define ITERATIONS 10000000

static void bench_overall() {
	struct timespec tstart, tend;
	clock_gettime(CLOCK_MONOTONIC_RAW, &tstart);
	for (int i = 0; i < ITERATIONS; i++) {
		int type = i % ULT_TYPE_MAX;
		ult_migrate(type);
	}
	clock_gettime(CLOCK_MONOTONIC_RAW, &tend);
	double result = ((double) tend.tv_sec - tstart.tv_sec) + (double) (tend.tv_nsec - tstart.tv_nsec) / 1e9;
	printf("%f s for %d migrations, %e s/iter\n", result, ITERATIONS, result / ITERATIONS);
}

static void bench_split() {
	struct timespec tstart, tend;
	double result[2] = {0};
	for (int i = 0; i < ITERATIONS; i++) {
		int type = i % ULT_TYPE_MAX;
		clock_gettime(CLOCK_MONOTONIC_RAW, &tstart);
		ult_migrate(type);
		clock_gettime(CLOCK_MONOTONIC_RAW, &tend);
		result[type] += ((double) tend.tv_sec - tstart.tv_sec) + (double) (tend.tv_nsec - tstart.tv_nsec) / 1e9;
	}
	printf("%f s for %d migrations, %e s/iter\n", result[0]+result[1], ITERATIONS, (result[0]+result[1]) / ITERATIONS);
	printf("slow->fast: %f s for %d migrations, %e s/iter\n", result[0], ITERATIONS/2, result[0]*2 / ITERATIONS);
	printf("fast->slow: %f s for %d migrations, %e s/iter\n", result[1], ITERATIONS/2, result[1]*2 / ITERATIONS);
}

int main(int argc, char **argv) {
	ult_register_klt();
	assert(ult_registered());
	ult_migrate(1);
	if (argc == 2) {
		if (strcmp(argv[1], "overall") == 0)
			bench_overall();
		if (strcmp(argv[1], "split") == 0)
			bench_split();
	} else {
		printf("Usage: %s <overall/split>\n", argv[0]);
		return 1;
	}
	ult_unregister_klt();
	return 0;
}
