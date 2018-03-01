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

/* Application analysis library that monitors performance counters between
 * developer-defined points. */

#include "swp.h"
#include "swp_util.h"

#include <likwid.h>

#include <inttypes.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <string>
#include <utility>

struct CtrState {
	double instructions = 0, cycles = 0, l2stat = 0, l3misses = 0;
	uint64_t calls = 0;
};

static std::map<std::pair<std::string, std::string>, CtrState> sections;
static std::string section_start;

// Likwid state
static int *cpulist;
static int group_id;

// Position in this list has to correspond to the Events enum.
static const char *event_str_ryzen = "RETIRED_INSTRUCTIONS:PMC0,CPU_CLOCKS_UNHALTED:PMC1,L2_LATENCY_CYCLES_WAIT_ON_FILLS:PMC2,L3_MISS:CPMC5";
static const char *event_str_skylake = "INSTR_RETIRED_ANY:FIXC0,CPU_CLK_UNHALTED_CORE:FIXC1,MEM_LOAD_RETIRED_L2_MISS:PMC0,MEM_LOAD_RETIRED_L3_MISS:PMC1";
enum class Events : int {
	instructions = 0,
	cycles,
	l2stat, // Ryzen: waiting latency (in 4 cycles), Skylake: misses
	l3misses,
};

static void print_sections() {
	// Needed to make thousands grouping work below.
	setlocale(LC_ALL, "");
	for (const auto& kv : sections) {
		const auto& section = kv.first;
		const auto& state = kv.second;
		printf("%s -> %s\n\tcalls = %'" PRIu64 "\n\tmiss rate = %f (l3miss = %'.0f / instr = %'.0f)\n\tCPI = %f\n\tL2 rate = %f\n",
				section.first.c_str(), section.second.c_str(),
				state.calls,
				state.l3misses / state.instructions,
				state.l3misses, state.instructions,
				state.cycles / state.instructions,
				state.l2stat / state.instructions);
	}
}

// Resets and starts counters.
static void init_counters(int group_id) {
	int err;
	err = perfmon_setupCounters(group_id);
	if (err < 0) {
		fprintf(stderr, "swp: Failed to setup group %d for thread %d\n", group_id, -err - 1);
		exit(-1);
	}
	err = perfmon_startCounters();
	if (err < 0) {
		fprintf(stderr, "swp: Failed to start counters for group %d for thread %d\n", group_id, -err - 1);
		exit(-1);
	}
}

extern "C" void swp_init() {
	section_start = "swp_init";

	// Initialize Likwid.
	int err;
	err = topology_init();
	if (err < 0) {
		fprintf(stderr, "Failed to initialize LIKWID's topology module\n");
		exit(-1);
	}
	CpuInfo_t info = get_cpuInfo();
	const char *event_str;
	if (!info->isIntel && info->family == 0x17)
		event_str = event_str_ryzen;
	else if (info->isIntel && info->family == 6)
		event_str = event_str_skylake;
	else {
		fprintf(stderr, "swp: Only AMD Ryzen or Intel Skylake CPUs supported\n");
		exit(-1);
	}
	CpuTopology_t topo = get_cpuTopology();
	cpulist = new int[topo->numHWThreads];
	//for (int i = 0; i < topo->numHWThreads; i++)
	//	cpulist[i] = topo->threadPool[i].apicId;
	// TODO: Properly support multiple threads.
	int cpu = sched_getcpu();
	likwid_pinProcess(cpu);
	cpulist[0] = topo->threadPool[cpu].apicId;
	//err = perfmon_init(topo->numHWThreads, cpulist);
	err = perfmon_init(1, cpulist);
	if (err < 0) {
		fprintf(stderr, "swp: Failed to initialize LIKWID's performance monitoring module\n");
		exit(-1);
	}
	group_id = perfmon_addEventSet(event_str);
	if (group_id < 0) {
		fprintf(stderr, "swp: Failed to add event string %s to LIKWID's performance monitoring module\n", event_str);
		exit(-1);
	}
	init_counters(group_id);
}

extern "C" void swp_mark(const char *id, const char *pos) {
	if (section_start.empty()) return;

	int err;
	err = perfmon_stopCounters();
	if (err < 0) {
		// This happens if swp_init() wasn't called on the same thread.
		fprintf(stderr, "swp_mark: Failed to stop counters for group %d for thread %d\n", group_id, -err - 1);
		return;
	}

	std::string section_end = swp::section_name(id, pos);
	auto& state = sections[{section_start, section_end}];
	state.calls++;
	state.instructions += perfmon_getLastResult(group_id, static_cast<int>(Events::instructions), 0);
	state.cycles += perfmon_getLastResult(group_id, static_cast<int>(Events::cycles), 0);
	state.l2stat += perfmon_getLastResult(group_id, static_cast<int>(Events::l2stat), 0);
	state.l3misses += perfmon_getLastResult(group_id, static_cast<int>(Events::l3misses), 0);
	section_start = std::move(section_end);

	init_counters(group_id);
}

extern "C" void swp_deinit() {
	swp_mark("swp_deinit", nullptr);

	delete[] cpulist;
	perfmon_finalize();
	topology_finalize();

	print_sections();
}
