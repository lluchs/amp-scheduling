
#ifndef ULTMIGRATRION_H_INCLUDED
#define ULTMIGRATRION_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ult_register_klt(void);
void ult_unregister_klt(void);
void ult_migrate(int phase);
int ult_registered(void);

#ifdef __cplusplus
}
#endif

#endif

