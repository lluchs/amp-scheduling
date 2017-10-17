#ifndef MICRO_H
#define MICRO_H

/* Configuration */

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

/* Declarations */

typedef int (*Fn)();
void run_benchmark(Fn littlework, Fn lotsofwork);

#endif
