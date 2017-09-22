#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include "ultmigration.h"

#define WORK 10000

static int lotsofwork() {
	unsigned int foo1 = 2, foo2 = 3, foo3 = 4, foo4 = 5;
	for (int i = 0; i < 100000; i++) {
		foo1 = foo1 * foo1; foo2 = foo2 * foo2; foo3 = foo3 * foo3; foo4 = foo4 * foo4;
		foo1 = foo1 * foo1; foo2 = foo2 * foo2; foo3 = foo3 * foo3; foo4 = foo4 * foo4;
	}
	return (foo1 + foo2 + foo3 + foo4) % 3;
}

static int littlework() {
	static const char buf[] = "foobar";
	int fd = open("/dev/null", O_WRONLY);
	for (int i = 0; i < 5000; i++) {
		write(fd, buf, sizeof buf);
	}
	close(fd);
	return 1;
}

int main() {
	ult_register_klt();
	int result = 0;
	for (int i = 0; i < WORK; i++) {
		ult_migrate(ULT_FAST);
		result += lotsofwork();
		ult_migrate(ULT_SLOW);
		result += littlework();
	}
	printf("done %d\n", result);
	ult_unregister_klt();
}
