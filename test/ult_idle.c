/* Initialize ultmigration, then do nothing. */

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
