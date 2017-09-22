
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

