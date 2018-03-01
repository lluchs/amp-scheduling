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

/* Initialize ultmigration, then do nothing.
 *
 * Used to test MWAIT overhead.
 */

#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ultmigration.h"

int main(int argc, char **argv) {
	ult_register_klt();
	// The argument is the core to MWAIT on.
	if (argc == 2) {
		if (strcmp(argv[1], "fast") == 0)
			ult_migrate(ULT_SLOW);
		else if (strcmp(argv[1], "slow") == 0)
			ult_migrate(ULT_FAST);
		else {
			printf("Usage: %s <core which MWAITs: fast/slow>\n", argv[0]);
			return 1;
		}
		printf("MWAIT on %s CPU\n", argv[1]);
	}
	printf("Pausing on CPU %d\n", sched_getcpu());
	pause();
	ult_unregister_klt();
}
