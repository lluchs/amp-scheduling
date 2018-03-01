amp-scheduling
==============

*Code I created for my Master's thesis. Abstract:*

Asymmetric multicore processors (AMP) integrate multiple core types with
different power and performance characteristics in a single package. Using
optimized scheduling, these processors can deliver higher performance per watt
than a symmetric multicore processor. An application executes most efficiently
on a certain core depending on how it uses resources like CPU and memory.
Previous approaches analyze applications at coarse granularities, classifying
each process or thread. In systems such as servers that have homogeneous
processes with similar behavior in all threads, these approaches cannot
distribute applications to core types effectively.

However, applications generally go through different execution phases over
time.  These phases often differ in their resource usage and exist at both
large and small scales. Whereas some systems already incorporate changing
application behavior over large time intervals, it should also be possible to
utilize shorter phases to save energy by migrating between cores at high
frequency.

In this work, we design and implement such a system that characterizes the
small-scale phase behavior of applications between developer-defined points by
monitoring memory accesses with performance counters. At runtime, it migrates
the application thread to the optimal core for each execution phase.

We evaluate our system on an AMD Ryzen processor. These processors allow
asymmetric core configurations using frequency scaling. We fail to see
reductions in power consumption with our system on these processors. We show
that, contrary to available documentation, Ryzen does not have per-core voltage
domains and conclude that these processors are not suitable as asymmetric
platform.

Building
--------

Most code is written in C and builds on Linux with the [Meson Build
system][meson]. Some parts require [likwid][] for performance counter
monitoring.

    meson build
	cd build
	ninja

Adapting an Application
-----------------------

0. Install the libraries globally with `ninja install`. This step isn't
   strictly necessary, but makes including and linking the libraries easier.

1. **Insert measurement points**. Include `swp.h` and add `SWP_MARK;` to
   functions. Call `swp_init()` during application initialization and
   `swp_deinit()` before shutdown.

2. **Run in measurement mode**. Modify the build system to link with libswp
   (i.e., add `-lswp` to the linker commands). Build and run the application.
   It will print results when `swp_deinit()` is called. Save the results in a
   file in `plot/out/swp`.

3. **Create control flow graph**. Run `make -C plot/out/swp`. It will create a
   graph in the same directory.

4. **Create application profile**. Run `plot/swpcfg.awk your-swp-output.txt`.
   It will print an application profile. Passing more than one file is also
   possible. Look at the profile and decide on a threshold value.

5. **Run in migration mode**. Run: 

        LD_PRELOAD=libswp_migrate.so SWP_CFG=your-swp-profile.txt SWP_THRESHOLD=0.42 FAST_CPU=0 SLOW_CPU=2 your-application

Benchmarks
----------

The repository includes three benchmark folders (`benchmark`, `ryzenpower`, and
`ultoverhead`) that work similarly. You need ZSH, GNU Awk, GNU Make, and R. To
run the benchmarks and analyze the results, do the following:

0. Build everything (see above). Enable user space `mwait` on Ryzen with
   `bin/enable-mwait`. In R, run `install.packages("tidyverse")` for the
   analysis and plotting scripts.
1. Modify the benchmark `run` script configuration at the top.
2. Run the benchmark: `./run <name of benchmark run>`
3. Wait for the benchmark to finish. Results are written to `results/<name>`.
4. Create TSV files by running `make tsv b=<name>` and plots by running
   `make plot b=<name>`.

The repository includes the benchmark runs I used for the thesis:

 - Figure 5.4 and 5.5: `benchmark/results/2018-02-22_01`
 - Figure 5.6 and 5.8: `ultoverhead/results/2018-02-21_02`
 - Figure 5.7: `ryzenpower/results/2018-02-25_01`
 - Figure 5.9a and A.1: `benchmark/results/2018-02-24_01`
 - Figure 5.9b and A.2: `benchmark/results/2018-02-21_01`

Repository Organization
-----------------------

### libultmigration

 - `ultmigration.[hsc]`: Implementation of user space core migration. Set the
   `FAST_CPU` and `SLOW_CPU` environment variables to the CPU ids you want to
   migrate to.

 - `ultmigration_dummy.c`: Dummy implementation for benchmarks or for systems
   that do not support user space `mwait`.

 - `ultmigration_pstate.c`: Implementation for Ryzen that changes the P-state
   of one core instead of migrating. Does not produce any useful results
   compared to migration and did not make it into the thesis.

### libswp

See “Adapting an Application” above for usage.

 - `swp/swp.h`: Common API for all swp libraries.

 - `swp/swp.cpp`: Application analysis library that monitors performance
   counters between developer-defined points. 

 - `swp/swp_migrate.cpp`: Library for migrating based on a profile and a
   threshold.

 - `swp/swp_dummy.cpp`: Dummy library for benchmarks.

### pmc

 - `pmc/pmc.c`: Small library for reading Ryzen L3 cache counters via MSRs.

### test

 - `test/simple.c`: Simple test for *libultmigration* that just migrates a few
   times.

 - `test/micro.c`: Microbenchmark modelling the optimal migration scenario. 

 - `test/micro_pmc.c`: *micro* with manual Ryzen L3 cache miss counter
   monitoring.

 - `test/micro_swp.c`: *micro* with *libswp* instrumentation.

 - `test/ult_idle.c`: Initializes *libultmigration*, the does nothing. Used to
   test `mwait` overhead.

 - `test/ultoverhead.c`: Benchmark for measuring performance overhead from
   *libultmigration*.

### tools

 - `tools/amdccx.c`: Reads Ryzen CCX id from cpuid data.

 - `tools/amdpstate.c`: Reads and writes Ryzen P-state configuration.
   Additional commands for reading effective frequency and RAPL counters.

 - `tools/l3topology.c`: Figures out core ids at the L3 cache on Ryzen
   processors with disabled cores. For example, on our Ryzen 1600X test system
   with six cores, the first L3 cache has cores 0, 2, 3 active and the second
   L3 cache cores 0, 1, 2. This information is important for filtering the L3
   cache counters by core.

[meson]: http://mesonbuild.com/
[likwid]: https://github.com/RRZE-HPC/likwid
