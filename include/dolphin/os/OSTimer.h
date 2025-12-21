/* TODO: not sure if this should live here or in libultra/OSTimer.h */

#ifndef DOLPHIN_OS_TIMER_H
#define DOLPHIN_OS_TIMER_H

#include "types.h"

#ifdef TARGET_PC
/* PC port - OSTimer is defined in os_compat.h */
#include "pc/os_compat.h"
#else
/* GameCube build */
#include "dolphin/os/OSAlarm.h"
#include "dolphin/os/OSMessage.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OSTimer_s {
    OSAlarm alarm;
    struct OSTimer_s* next;
    struct OSTimer_s* prev;
    OSTime interval;
    OSTime value;
    OSMessageQueue* mq;
    OSMessage msg;
} OSTimer;

#ifdef __cplusplus
}
#endif

#endif /* !TARGET_PC */
#endif /* DOLPHIN_OS_TIMER_H */
