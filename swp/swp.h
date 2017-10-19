/* Switching point API */

#ifndef SWP_H
#define SWP_H

#ifdef __cplusplus
extern "C" {
#endif

#define SWP_STRINGIZE(x) SWP_DO_STRINGIZE(x)
#define SWP_DO_STRINGIZE(x) #x
#define SWP_MARK swp_mark(__func__, __FILE__ ":" SWP_STRINGIZE(__LINE__))

void swp_init();
void swp_mark(const char *id, const char *pos);
void swp_deinit();

#ifdef __cplusplus
}
#endif

#endif
