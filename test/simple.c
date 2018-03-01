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

/* Simple test for libultmigration that just migrates a few times. */

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
