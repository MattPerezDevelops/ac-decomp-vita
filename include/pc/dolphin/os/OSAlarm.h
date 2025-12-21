/**
 * @file OSAlarm.h
 * @brief Stub OSAlarm for PC port
 *
 * The PC port doesn't need hardware timer alarms - just provide
 * the typedef so structures compile.
 */
#ifndef PC_DOLPHIN_OSALARM_H
#define PC_DOLPHIN_OSALARM_H

#include "pc/platform.h"
#include "pc/dolphin/os/OSContext.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OSAlarm OSAlarm;
typedef void (*OSAlarmHandler)(OSAlarm* alarm, OSContext* context);

struct OSAlarm {
    OSAlarmHandler handler;
    u64 fire;           /* OSTime */
    OSAlarm *prev;
    OSAlarm *next;
    u64 period;         /* OSTime */
    u64 start;          /* OSTime */
};

/* Stub functions */
static inline void OSInitAlarm(void) { }
static inline void OSSetAlarm(OSAlarm *alarm, u64 tick, OSAlarmHandler handler) {
    (void)alarm; (void)tick; (void)handler;
}
static inline void OSSetAbsAlarm(OSAlarm *alarm, u64 time, OSAlarmHandler handler) {
    (void)alarm; (void)time; (void)handler;
}
static inline void OSSetPeriodicAlarm(OSAlarm *alarm, u64 start, u64 period, OSAlarmHandler handler) {
    (void)alarm; (void)start; (void)period; (void)handler;
}
static inline void OSCreateAlarm(OSAlarm *alarm) { (void)alarm; }
static inline void OSCancelAlarm(OSAlarm *alarm) { (void)alarm; }
static inline int OSCheckAlarmQueue(void) { return 0; }

#ifdef __cplusplus
}
#endif

#endif /* PC_DOLPHIN_OSALARM_H */
