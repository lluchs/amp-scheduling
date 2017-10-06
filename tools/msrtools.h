#ifndef MSRTOOLS_H
#define MSRTOOLS_H

#include <inttypes.h>

void init_dev_msr();
void deinit_dev_msr();

uint64_t rdmsr_on_cpu(uint32_t reg, int cpu);
void wrmsr_on_cpu(uint32_t reg, int cpu, uint64_t data);
void wrmsr_on_all_cpus(uint32_t reg, uint64_t data);

#endif

