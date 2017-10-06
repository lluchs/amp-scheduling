/* ----------------------------------------------------------------------- *
 *
 *   Contains code from msr-tools (https://github.com/01org/msr-tools)
 *   under the following license: 
 *
 *   Copyright 2000 Transmeta Corporation - All Rights Reserved
 *   Copyright 2004-2008 H. Peter Anvin - All Rights Reserved
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *   Boston MA 02110-1301, USA; either version 2 of the License, or
 *   (at your option) any later version; incorporated herein by reference.
 *
 * ----------------------------------------------------------------------- */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

static int msr_fds[64];

uint64_t rdmsr_on_cpu(uint32_t reg, int cpu)
{
	uint64_t data;
	int fd;
	char msr_file_name[64];

	fd = msr_fds[cpu];
	if (!fd) {
		sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
		fd = open(msr_file_name, O_RDONLY);
		if (fd < 0) {
			if (errno == ENXIO) {
				fprintf(stderr, "rdmsr: No CPU %d\n", cpu);
				exit(2);
			} else if (errno == EIO) {
				fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n",
					cpu);
				exit(3);
			} else {
				perror("rdmsr: open");
				exit(127);
			}
		}
	}

	if (pread(fd, &data, sizeof data, reg) != sizeof data) {
		if (errno == EIO) {
			fprintf(stderr, "rdmsr: CPU %d cannot read "
				"MSR 0x%08"PRIx32"\n",
				cpu, reg);
			exit(4);
		} else {
			perror("rdmsr: pread");
			exit(127);
		}
	}

	if (!msr_fds[cpu])
		close(fd);

	return data;
}

void wrmsr_on_cpu(uint32_t reg, int cpu, uint64_t data)
{
	int fd;
	char msr_file_name[64];

	fd = msr_fds[cpu];
	if (!fd) {
		sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
		fd = open(msr_file_name, O_WRONLY);
		if (fd < 0) {
			if (errno == ENXIO) {
				fprintf(stderr, "wrmsr: No CPU %d\n", cpu);
				exit(2);
			} else if (errno == EIO) {
				fprintf(stderr, "wrmsr: CPU %d doesn't support MSRs\n",
					cpu);
				exit(3);
			} else {
				perror("wrmsr: open");
				exit(127);
			}
		}
	}

	if (pwrite(fd, &data, sizeof data, reg) != sizeof data) {
		if (errno == EIO) {
			fprintf(stderr,
				"wrmsr: CPU %d cannot set MSR "
				"0x%08"PRIx32" to 0x%016"PRIx64"\n",
				cpu, reg, data);
			exit(4);
		} else {
			perror("wrmsr: pwrite");
			exit(127);
		}
	}

	if (!msr_fds[cpu])
		close(fd);

	return;
}

/* filter out ".", "..", "microcode" in /dev/cpu */
static int dir_filter(const struct dirent *dirp) {
	if (isdigit(dirp->d_name[0]))
		return 1;
	else
		return 0;
}

void wrmsr_on_all_cpus(uint32_t reg, uint64_t data)
{
	struct dirent **namelist;
	int dir_entries;

	dir_entries = scandir("/dev/cpu", &namelist, dir_filter, 0);
	while (dir_entries--) {
		wrmsr_on_cpu(reg, atoi(namelist[dir_entries]->d_name), data);
		free(namelist[dir_entries]);
	}
	free(namelist);
}

void init_dev_msr()
{
	struct dirent **namelist;
	int dir_entries, cpu, fd;
	char msr_file_name[64];

	dir_entries = scandir("/dev/cpu", &namelist, dir_filter, 0);
	while (dir_entries--) {
		cpu = atoi(namelist[dir_entries]->d_name);
		assert(cpu < sizeof(msr_fds) / sizeof(msr_fds[0]));

		sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
		fd = open(msr_file_name, O_RDWR);
		if (fd < 0) {
			if (errno == ENXIO) {
				fprintf(stderr, "msr: No CPU %d\n", cpu);
				exit(2);
			} else if (errno == EIO) {
				fprintf(stderr, "msr: CPU %d doesn't support MSRs\n",
					cpu);
				exit(3);
			} else {
				perror("msr: open");
				exit(127);
			}
		}
		msr_fds[cpu] = fd;

		free(namelist[dir_entries]);
	}
	free(namelist);
}

void deinit_dev_msr()
{
	int i; 
	for (i = 0; i < sizeof(msr_fds) / sizeof(msr_fds[0]); i++) {
		if (msr_fds[i]) {
			close(msr_fds[i]);
			msr_fds[i] = 0;
		}
	}
}
