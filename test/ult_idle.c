/* Initialize ultmigration, then do nothing. */

#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ultmigration.h"

int main(int argc, char **argv) {
	ult_register_klt();
	if (argc == 2) {
		if (strcmp(argv[1], "fast") == 0)
			ult_migrate(ULT_FAST);
		else if (strcmp(argv[1], "slow") == 0)
			ult_migrate(ULT_SLOW);
	}
	printf("Idling on CPU %d\n", sched_getcpu());
	pause();
	ult_unregister_klt();
}
