#include <stdio.h>
#include <sched.h>
#include "ultmigration.h"

int main() {
	ult_register_klt();
	for (int i = 0; i < 10; i++) {
		int cpu = sched_getcpu();
		int phase = i % 4;
		ult_migrate(phase);
		printf("ult_migrate(%d): CPU %d -> %d\n", phase, cpu, sched_getcpu());
	}
	ult_unregister_klt();
}
