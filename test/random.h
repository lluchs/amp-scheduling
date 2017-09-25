#ifndef RANDOM_H
#define RANDOM_H

#include <unistd.h>
#include <stdio.h>

static __uint128_t random_state;

static void random_init() {
	if (getentropy(&random_state, sizeof random_state) < 0) {
		perror("getentropy");
		exit(-1);
	}
}

static inline uint64_t random_next() {
	random_state *= UINT64_C(0xda942042e4dd58b5);
	return random_state >> 64;
}

#endif
