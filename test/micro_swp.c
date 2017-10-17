#include <stdio.h>

#include "ultmigration.h"
#include "swp/swp.h"
#include "micro.h"

void run_benchmark(Fn littlework, Fn lotsofwork) {
	swp_init();

	int result = 0;
	for (int i = 0; i < WORK; i++) {
		//ult_migrate(ULT_FAST);
		SWP_MARK;
		result += lotsofwork();

		//ult_migrate(ULT_SLOW);
		SWP_MARK;
		result += littlework();
	}
	printf("done %d\n", result);

	swp_deinit();
}
