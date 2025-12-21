#ifndef LIBULTRA_H
#define LIBULTRA_H

#include "types.h"

#ifdef TARGET_PC
/* PC port - use PC compatibility layer */
#include "pc/os_compat.h"
#include "libultra/gu.h"   /* graphics utility functions */
#include "libc64/math64.h" /* sins and coss */
#include <string.h>  /* for memset/memmove */

#define N64_SCREEN_HEIGHT 240
#define N64_SCREEN_WIDTH 320

typedef u64 Z_OSTime;

/* bzero/bcopy/bcmp - provided by strings.h via string.h */
#include <strings.h>

/* Additional OS functions */
static inline void osSyncPrintf(const char* fmt, ...) { (void)fmt; }
static inline void osWritebackDCache(void* vaddr, u32 nbytes) { (void)vaddr; (void)nbytes; }

#else
/* GameCube/N64 build - use original headers */
#include "dolphin/os/OSTime.h"
#include "dolphin/os/OSCache.h"
#include "libultra/gu.h"
#include "libultra/osMesg.h"
#include "libultra/shutdown.h"
#include "libultra/os_timer.h"
#include "libultra/os_thread.h"
#include "libultra/os_pi.h"
#include "libultra/initialize.h"
#include "libc64/math64.h" /* TODO: sins and coss belong in libultra */

#define N64_SCREEN_HEIGHT 240
#define N64_SCREEN_WIDTH 320

#ifdef __cplusplus
extern "C" {
#endif

typedef u64 Z_OSTime;

int bcmp(void* v1, void* v2, u32 size);
void bcopy(void* src, void* dst, size_t n);
void bzero(void* ptr, size_t size);
void osSyncPrintf(const char* fmt, ...);
void osWritebackDCache(void* vaddr, u32 nbytes);
u32 osGetCount(void);
OSTime osGetTime(void);

extern s32 osAppNMIBuffer[16];
extern int osShutdown;
extern u8 __osResetSwitchPressed;

#ifdef __cplusplus
}
#endif

#endif /* !TARGET_PC */
#endif /* LIBULTRA_H */
