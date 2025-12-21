/**
 * @file os_compat.h
 * @brief N64 OS API compatibility layer for PC port
 *
 * This header provides N64-style OS function declarations that are
 * implemented using SDL2 and standard library functions.
 *
 * Part of Layer 2 (Porting Abstraction Layer).
 */
#ifndef PC_OS_COMPAT_H
#define PC_OS_COMPAT_H

#include "platform.h"

/* ============================================================================
 * Thread Types and Definitions
 * ============================================================================ */

typedef s32 OSPri;
typedef s32 OSId;
typedef u64 OSTime;

/* Thread priorities */
#define OS_PRIORITY_IDLE      0
#define OS_PRIORITY_APPMAX    127
#define OS_PRIORITY_VIMGR     140
#define OS_PRIORITY_RMON      250
#define OS_PRIORITY_MAX       255

/* Forward declare OSThread for queue */
typedef struct OSThread OSThread;

/* Thread queue (used by blocking operations) */
typedef struct OSThreadQueue {
    OSThread* head;
    OSThread* tail;
} OSThreadQueue;

/* Thread structure for PC port (SDL-based) */
struct OSThread {
    void* sdl_thread;           /* SDL_Thread* */
    void* mutex;                /* SDL_mutex* */
    void* cond;                 /* SDL_cond* */
    void (*entry)(void*);       /* Thread entry function */
    void* arg;                  /* Thread argument */
    OSId id;                    /* Thread ID */
    OSPri priority;             /* Thread priority */
    int running;                /* Running flag */
    int stopped;                /* Stopped flag */
};

/* ============================================================================
 * Message Queue Types
 * ============================================================================ */

typedef void* OSMesg;

typedef struct OSMesgQueue {
    /* Original N64 fields (for compatibility) */
    OSThread* mtqueue;      /* Queue of threads blocked on receive */
    OSThread* fullqueue;    /* Queue of threads blocked on send */
    s32 validCount;         /* Number of valid messages */
    s32 first;              /* First valid message index */
    s32 msgCount;           /* Total message slots */
    OSMesg* msg;            /* Message buffer */

    /* PC-specific synchronization primitives */
    void* mutex;            /* SDL_mutex* for this queue */
    void* cond_not_empty;   /* SDL_cond* signaled when queue becomes non-empty */
    void* cond_not_full;    /* SDL_cond* signaled when queue becomes non-full */
} OSMesgQueue;

/* Message flags */
#define OS_MESG_NOBLOCK 0
#define OS_MESG_BLOCK   1

/* ============================================================================
 * Timer Types
 * ============================================================================ */

typedef struct OSTimer {
    struct OSTimer* next;
    struct OSTimer* prev;
    OSTime interval;
    OSTime remaining;
    OSMesgQueue* mq;
    OSMesg msg;
} OSTimer;

/* ============================================================================
 * Event Types (for VI, SI, PI interrupts)
 * ============================================================================ */

#define OS_EVENT_SW1          0
#define OS_EVENT_SW2          1
#define OS_EVENT_CART         2
#define OS_EVENT_COUNTER      3
#define OS_EVENT_SP           4
#define OS_EVENT_SI           5
#define OS_EVENT_AI           6
#define OS_EVENT_VI           7
#define OS_EVENT_PI           8
#define OS_EVENT_DP           9
#define OS_EVENT_CPU_BREAK    10
#define OS_EVENT_SP_BREAK     11
#define OS_EVENT_FAULT        12
#define OS_EVENT_THREADSTATUS 13

/* ============================================================================
 * Controller Types
 * ============================================================================ */

/* Controller button masks */
#define CONT_A          0x8000
#define CONT_B          0x4000
#define CONT_G          0x2000
#define CONT_START      0x1000
#define CONT_UP         0x0800
#define CONT_DOWN       0x0400
#define CONT_LEFT       0x0200
#define CONT_RIGHT      0x0100
#define CONT_L          0x0020
#define CONT_R          0x0010
#define CONT_E          0x0008
#define CONT_D          0x0004
#define CONT_C_UP       0x0008
#define CONT_C_DOWN     0x0004
#define CONT_C_LEFT     0x0002
#define CONT_C_RIGHT    0x0001

/* Controller status */
#define CONT_CARD_ON        0x01
#define CONT_CARD_PULL      0x02
#define CONT_ADDR_CRC_ER    0x04
#define CONT_EEPROM         0x80
#define CONT_NO_RESPONSE_ERROR  0x8
#define CONT_OVERRUN_ERROR      0x4

/* Controller pad data */
typedef struct {
    u16 button;     /* Button state */
    s8 stick_x;     /* Analog stick X (-80 to 80) */
    s8 stick_y;     /* Analog stick Y (-80 to 80) */
    u8 errno;       /* Error number */
} OSContPad;

/* Controller status */
typedef struct {
    u16 type;       /* Controller type */
    u8 status;      /* Controller status */
    u8 errno;       /* Error number */
} OSContStatus;

/* Maximum controllers */
#define MAXCONTROLLERS 4

/* ============================================================================
 * Function Declarations - Threading
 * ============================================================================ */

void osCreateThread(OSThread* t, OSId id, void (*entry)(void*),
                    void* arg, void* sp, OSPri pri);
void osDestroyThread(OSThread* t);
void osStartThread(OSThread* t);
void osStopThread(OSThread* t);
void osSetThreadPri(OSThread* t, OSPri pri);
OSPri osGetThreadPri(OSThread* t);
OSId osGetThreadId(OSThread* t);
void osYieldThread(void);
OSThread* osGetCurrentThread(void);

/* ============================================================================
 * Function Declarations - Message Queues
 * ============================================================================ */

void osCreateMesgQueue(OSMesgQueue* mq, OSMesg* msg, s32 count);
s32 osSendMesg(OSMesgQueue* mq, OSMesg msg, s32 flags);
s32 osRecvMesg(OSMesgQueue* mq, OSMesg* msg, s32 flags);
s32 osJamMesg(OSMesgQueue* mq, OSMesg msg, s32 flags);

/* ============================================================================
 * Function Declarations - Timing
 * ============================================================================ */

OSTime osGetTime(void);
u32 osGetCount(void);
void osSetTime(OSTime time);

/* Timer functions */
void osSetTimer(OSTimer* t, OSTime countdown, OSTime interval,
                OSMesgQueue* mq, OSMesg msg);
s32 osStopTimer(OSTimer* t);

/* ============================================================================
 * Function Declarations - Interrupts (mostly stubs on PC)
 * ============================================================================ */

s32 osSetIntMask(s32 mask);
void osSetEventMesg(s32 event, OSMesgQueue* mq, OSMesg msg);

/* ============================================================================
 * Function Declarations - Controller
 * ============================================================================ */

s32 osContInit(OSMesgQueue* mq, u8* bitpattern, OSContStatus* status);
s32 osContStartQuery(OSMesgQueue* mq);
s32 osContStartReadData(OSMesgQueue* mq);
s32 osContGetQuery(OSContStatus* status);
void osContGetReadData(OSContPad* pad);

/* ============================================================================
 * Function Declarations - Memory
 * ============================================================================ */

/* These are in mem_compat.h but declared here for convenience */
void* osAlloc(size_t size);
void osFree(void* ptr);

/* ============================================================================
 * Function Declarations - Initialization
 * ============================================================================ */

/* Initialize OS compatibility layer (call at startup) */
void osInitialize(void);
void pc_osShutdown(void);   /* PC cleanup - call on exit */
void __osInitialize_common(void);

/* ============================================================================
 * Shutdown/Reset System
 * ============================================================================ */

/* Reset types */
#define OS_RESET_SHUTDOWN   0
#define OS_RESET_RESTART    1
#define OS_RESET_NMI        2
#define OS_RESET_HOTRESET   3

/* osShutdown variable - indicates shutdown state */
extern int osShutdown;
extern s32 osAppNMIBuffer[16];
extern u8 __osResetSwitchPressed;

/* Shutdown functions */
void osShutdownStart(int type);

/* NMI buffer indices */
#define APPNMI_FLAGS_IDX 0

#endif /* PC_OS_COMPAT_H */
