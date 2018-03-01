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
#ifndef AMDCCX_H
#define AMDCCX_H

#include <inttypes.h>

// See 2.1.10.2.1.3 ApicId Enumeration Requirements
union ApicId {
	struct {
		// SMT => first bit is thread id
		unsigned CoreAndThreadId:3;
		// This is the interesting bit. Cores with the same CCXID share the L3
		// cache, otherwise they are connected via a cache coherent fabric.
		unsigned CCXID:1;
		unsigned NodeID:2;
		unsigned SocketID:1;
	};
	uint32_t value;
};

union ApicId apicid_on_cpu(int cpu);

#endif
