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

/* Tool for reading Ryzen CCX id from cpuid information. */

#include "amdccx.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const uint32_t ExtApicId = 0x8000001e;

union ApicId apicid_on_cpu(int cpu) {
	char cpuid_file_name[64];

	sprintf(cpuid_file_name, "/dev/cpu/%d/cpuid", cpu);
	int fd = open(cpuid_file_name, O_RDONLY);
	if (fd < 0) {
		if (errno == ENXIO) {
			fprintf(stderr, "apicid: No CPU %d\n", cpu);
			exit(2);
		}  else {
			perror("apicid: open");
			exit(127);
		}
	}

	uint32_t regs[4];
	if (pread(fd, &regs, sizeof regs, ExtApicId) != sizeof regs) {
		if (errno == EIO) {
			fprintf(stderr, "apicid: CPU %d cannot read cpuid", cpu);
			exit(4);
		} else {
			perror("apicid: pread");
			exit(127);
		}
	}

	close(fd);

	return (union ApicId) {.value = regs[0]};
}
