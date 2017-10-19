#include "swp.h"

#include <likwid.h>

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <string>
#include <utility>

struct CtrState {
	double instructions = 0, cache_misses = 0;
};

static std::map<std::pair<std::string, std::string>, CtrState> sections;
static std::string section_start;

// Likwid state
static int *cpulist;
static int group_id;

// Position in this list has to correspond to the Events enum.
static const char *event_str_ryzen = "INST_RETIRED_ANY:FIXC0,L3_MISS:CPMC5";
static const char *event_str_skylake = "INSTR_RETIRED_ANY:FIXC0,MEM_LOAD_RETIRED_L3_MISS:PMC0";
enum class Events : int {
	instructions = 0,
	cache_misses,
};

static void print_sections() {
	// Needed to make thousands grouping work below.
	setlocale(LC_ALL, "");
	for (const auto& kv : sections) {
		const auto& section = kv.first;
		const auto& state = kv.second;
		printf("%s -> %s miss rate = %f (l3miss = %'.0f / instr = %'.0f)\n",
				section.first.c_str(), section.second.c_str(),
				state.cache_misses / state.instructions,
				state.cache_misses, state.instructions);
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
	section_start = "__start";

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

extern "C" void swp_mark(const char *id) {
	int err;
	err = perfmon_stopCounters();
	if (err < 0) {
		fprintf(stderr, "swp: Failed to stop counters for group %d for thread %d\n", group_id, -err - 1);
		exit(-1);
	}

	std::string section_end = id;
	auto& state = sections[{section_start, section_end}];
	state.instructions += perfmon_getLastResult(group_id, static_cast<int>(Events::instructions), 0);
	state.cache_misses += perfmon_getLastResult(group_id, static_cast<int>(Events::cache_misses), 0);
	section_start = std::move(section_end);

	init_counters(group_id);
}

extern "C" void swp_deinit() {
	swp_mark("__end");

	delete[] cpulist;
	perfmon_finalize();
	topology_finalize();

	print_sections();
}
