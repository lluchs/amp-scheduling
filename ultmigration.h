/*
 * Copyright © 2017, Mathias Gottschlag
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

#ifndef ULTMIGRATRION_H_INCLUDED
#define ULTMIGRATRION_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ult_thread_type {
	ULT_FAST = 0,
	ULT_SLOW,
	ULT_TYPE_MAX
};

void ult_register_klt(void);
void ult_unregister_klt(void);
void ult_migrate(enum ult_thread_type);
int ult_registered(void);

#ifdef __cplusplus
}
#endif

#endif

