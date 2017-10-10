/* l3topology
 *
 * All Ryzen systems have two CCX with four cores / eight threads each. CPUs
 * with less than eight cores have one or two cores from each CCX disabled to
 * get a four/six core system.
 *
 * This isn't exposed anywhere (as far as I can tell), except for the L3 cache
 * counters which allow selection of the eight possible threads. On an
 * eight-core system, this directly corresponds to the core-and-thread-ID, but
 * on systems with less cores, there may be a gap.
 *
 * This program produces cache misses while observing the L3 cache counters to
 * figure out which cores are disabled.
 */
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "amdccx.h"
#include "../pmc/pmc.h"
#include "../test/random.h"

void setup_pmc(int cpu) {
	union pmc_l3_event pmc = {
		.EventSel = L3Miss,
		.UnitMask = L3MissUMask,
		.Enable = 1,
		.SliceMask = 0xf,
	};
	for (int core = 0; core < 4; core++) {
		// Select both threads of a core. There are only six counters, so we
		// can't have a counter for all eight threads.
		pmc.ThreadMask = 0x3 << (core * 2);
		pmc_select_l3_event(cpu, core, pmc);
	}
	// Use the other two counters to differentiate between threads.
	pmc.ThreadMask = 0x55; // Thread 0
	pmc_select_l3_event(cpu, 4, pmc);
	pmc.ThreadMask = 0xaa; // Thread 1
	pmc_select_l3_event(cpu, 5, pmc);
}

void reset_pmc(int cpu) {
	for (int ctr = 0; ctr < 6; ctr++) {
		pmc_write_l3_counter(cpu, ctr, 0);
	}
}

void set_affinity(int cpu) {
	cpu_set_t cpus;
	CPU_ZERO(&cpus);
	CPU_SET(cpu, &cpus);
	sched_setaffinity(0, sizeof(cpus), &cpus);
}

// Pointer Chasing from test/micro {{{
#define BUFSIZE (size_t) (100 << 20)
#define CH_BUFLEN (BUFSIZE / sizeof(void*))
#define POINTER_CHASE_STEPS 50000
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
// }}}

int main() {
	random_init();
	init_pointer_chasing();

	int cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
	// Setup counters on both CCX.
	setup_pmc(0); setup_pmc(cpu_count - 1);
	for (int cpu = 0; cpu < cpu_count; cpu++) {
		union ApicId aid = apicid_on_cpu(cpu);
		set_affinity(cpu);
		reset_pmc(cpu);
		// Cause lots of L3 cache misses.
		pointer_chasing();
		// Find the counter with the largest number of misses.
		uint64_t max = 0, ctr, t0, t1;
		int maxcore = -1, thread;
		for (int core = 0; core < 4; core++) {
			ctr = pmc_read_l3_counter(core);
			if (ctr > max) {
				max = ctr;
				maxcore = core;
			}
		}
		t0 = pmc_read_l3_counter(4);
		t1 = pmc_read_l3_counter(5);
		thread = t0 > t1 ? 0 : 1;
		printf("CPU %2d: CCX %d CCX-Core %d L3-Core %d L3-Thread %d\n", cpu, aid.CCXID, aid.CoreAndThreadId, maxcore, thread);
	}
}

// vim: fdm=marker
