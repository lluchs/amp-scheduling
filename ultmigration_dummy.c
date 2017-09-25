#include "ultmigration.h"

static int registered = 0;

void ult_register_klt(void) { registered = 1; }
void ult_unregister_klt(void) { registered = 0; }
void ult_migrate(enum ult_thread_type type) { }
int ult_registered(void) { return registered; }

