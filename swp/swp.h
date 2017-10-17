/* Switching point API */

#ifndef SWP_H
#define SWP_H

#ifdef __cplusplus
extern "C" {
#endif

#define SWP_MARK swp_mark(__FILE__ ":" __LINE__)

void swp_init();
void swp_mark(const char *id);
void swp_deinit();

#ifdef __cplusplus
}
#endif

#endif
