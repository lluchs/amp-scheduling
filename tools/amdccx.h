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
