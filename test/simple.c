#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <sched.h>
#include "ultmigration.h"

int main() {
	ult_register_klt();
	assert(ult_registered());
	for (int i = 0; i < 10; i++) {
		int cpu = sched_getcpu();
		int type = i % ULT_TYPE_MAX;
		ult_migrate(type);
		printf("ult_migrate(%d): CPU %d -> %d\n", type, cpu, sched_getcpu());
	}
	ult_unregister_klt();
}
