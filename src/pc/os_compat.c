/**
 * @file os_compat.c
 * @brief N64 OS API implementation using SDL2
 *
 * Part of Layer 2 (Porting Abstraction Layer).
 * Implements N64-style OS functions using SDL2 and standard library.
 *
 * Key design decisions:
 * - Each message queue has its own mutex and condition variables
 * - Blocking operations use proper condition variable waits
 * - Threads are implemented with SDL2 threads
 * - Timers run in a background thread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "pc/os_compat.h"

/* ============================================================================
 * Thread Implementation
 * ============================================================================ */

/* Thread-local storage for current thread tracking */
static SDL_TLSID g_current_thread_tls = 0;

/* Thread entry wrapper */
static int thread_entry_wrapper(void* data) {
    OSThread* t = (OSThread*)data;

    /* Set thread-local current thread */
    SDL_TLSSet(g_current_thread_tls, t, NULL);

    if (t->entry) {
        t->entry(t->arg);
    }

    t->running = 0;
    return 0;
}

void osCreateThread(OSThread* t, OSId id, void (*entry)(void*),
                    void* arg, void* sp, OSPri pri) {
    (void)sp; /* Stack pointer not used on PC */

    memset(t, 0, sizeof(OSThread));
    t->id = id;
    t->priority = pri;
    t->entry = entry;
    t->arg = arg;
    t->running = 0;
    t->stopped = 1;
    t->mutex = SDL_CreateMutex();
    t->cond = SDL_CreateCond();
}

void osDestroyThread(OSThread* t) {
    if (t->sdl_thread) {
        SDL_WaitThread(t->sdl_thread, NULL);
        t->sdl_thread = NULL;
    }
    if (t->mutex) {
        SDL_DestroyMutex(t->mutex);
        t->mutex = NULL;
    }
    if (t->cond) {
        SDL_DestroyCond(t->cond);
        t->cond = NULL;
    }
}

void osStartThread(OSThread* t) {
    if (!t->running && t->stopped) {
        t->running = 1;
        t->stopped = 0;
        char name[32];
        snprintf(name, sizeof(name), "os_thread_%d", t->id);
        t->sdl_thread = SDL_CreateThread(thread_entry_wrapper, name, t);
        if (t->sdl_thread) {
            /* Set priority hint if possible */
            SDL_SetThreadPriority(
                t->priority >= OS_PRIORITY_VIMGR ? SDL_THREAD_PRIORITY_HIGH :
                t->priority >= OS_PRIORITY_APPMAX ? SDL_THREAD_PRIORITY_NORMAL :
                SDL_THREAD_PRIORITY_LOW
            );
        }
    }
}

void osStopThread(OSThread* t) {
    SDL_LockMutex(t->mutex);
    t->stopped = 1;
    SDL_CondBroadcast(t->cond);
    SDL_UnlockMutex(t->mutex);
}

void osSetThreadPri(OSThread* t, OSPri pri) {
    t->priority = pri;
    /* Note: SDL doesn't support per-thread priority changes after creation */
}

OSPri osGetThreadPri(OSThread* t) {
    return t->priority;
}

OSId osGetThreadId(OSThread* t) {
    return t->id;
}

void osYieldThread(void) {
    SDL_Delay(0);
}

OSThread* osGetCurrentThread(void) {
    if (g_current_thread_tls == 0) {
        return NULL;
    }
    return (OSThread*)SDL_TLSGet(g_current_thread_tls);
}

/* ============================================================================
 * Message Queue Implementation
 *
 * Each queue has its own mutex and condition variables for proper blocking.
 * This avoids the global mutex bottleneck and allows efficient waiting.
 * ============================================================================ */

void osCreateMesgQueue(OSMesgQueue* mq, OSMesg* msg, s32 count) {
    memset(mq, 0, sizeof(OSMesgQueue));
    mq->msg = msg;
    mq->msgCount = count;
    mq->validCount = 0;
    mq->first = 0;

    /* Create per-queue synchronization primitives */
    mq->mutex = SDL_CreateMutex();
    mq->cond_not_empty = SDL_CreateCond();
    mq->cond_not_full = SDL_CreateCond();
}

void osDestroyMesgQueue(OSMesgQueue* mq) {
    if (mq->mutex) {
        SDL_DestroyMutex(mq->mutex);
        mq->mutex = NULL;
    }
    if (mq->cond_not_empty) {
        SDL_DestroyCond(mq->cond_not_empty);
        mq->cond_not_empty = NULL;
    }
    if (mq->cond_not_full) {
        SDL_DestroyCond(mq->cond_not_full);
        mq->cond_not_full = NULL;
    }
}

s32 osSendMesg(OSMesgQueue* mq, OSMesg msg, s32 flags) {
    SDL_mutex* mutex = (SDL_mutex*)mq->mutex;
    SDL_cond* cond_not_empty = (SDL_cond*)mq->cond_not_empty;
    SDL_cond* cond_not_full = (SDL_cond*)mq->cond_not_full;

    /* Handle case where queue wasn't properly initialized (legacy code) */
    if (!mutex) {
        /* Fallback: just add message if there's space */
        if (mq->validCount >= mq->msgCount) {
            return (flags == OS_MESG_NOBLOCK) ? -1 : 0;
        }
        s32 index = (mq->first + mq->validCount) % mq->msgCount;
        mq->msg[index] = msg;
        mq->validCount++;
        return 0;
    }

    SDL_LockMutex(mutex);

    /* Wait for space if queue is full */
    while (mq->validCount >= mq->msgCount) {
        if (flags == OS_MESG_NOBLOCK) {
            SDL_UnlockMutex(mutex);
            return -1;
        }
        /* Block until space available */
        SDL_CondWait(cond_not_full, mutex);
    }

    /* Add message to end of queue */
    s32 index = (mq->first + mq->validCount) % mq->msgCount;
    mq->msg[index] = msg;
    mq->validCount++;

    /* Signal that queue is no longer empty */
    SDL_CondSignal(cond_not_empty);

    SDL_UnlockMutex(mutex);
    return 0;
}

s32 osRecvMesg(OSMesgQueue* mq, OSMesg* msg, s32 flags) {
    SDL_mutex* mutex = (SDL_mutex*)mq->mutex;
    SDL_cond* cond_not_empty = (SDL_cond*)mq->cond_not_empty;
    SDL_cond* cond_not_full = (SDL_cond*)mq->cond_not_full;

    /* Handle case where queue wasn't properly initialized (legacy code) */
    if (!mutex) {
        if (mq->validCount <= 0) {
            return (flags == OS_MESG_NOBLOCK) ? -1 : 0;
        }
        if (msg) {
            *msg = mq->msg[mq->first];
        }
        mq->first = (mq->first + 1) % mq->msgCount;
        mq->validCount--;
        return 0;
    }

    SDL_LockMutex(mutex);

    /* Wait for message if queue is empty */
    while (mq->validCount <= 0) {
        if (flags == OS_MESG_NOBLOCK) {
            SDL_UnlockMutex(mutex);
            return -1;
        }
        /* Block until message available */
        SDL_CondWait(cond_not_empty, mutex);
    }

    /* Get message from front of queue */
    if (msg) {
        *msg = mq->msg[mq->first];
    }
    mq->first = (mq->first + 1) % mq->msgCount;
    mq->validCount--;

    /* Signal that queue is no longer full */
    SDL_CondSignal(cond_not_full);

    SDL_UnlockMutex(mutex);
    return 0;
}

s32 osJamMesg(OSMesgQueue* mq, OSMesg msg, s32 flags) {
    SDL_mutex* mutex = (SDL_mutex*)mq->mutex;
    SDL_cond* cond_not_empty = (SDL_cond*)mq->cond_not_empty;
    SDL_cond* cond_not_full = (SDL_cond*)mq->cond_not_full;

    if (!mutex) {
        if (mq->validCount >= mq->msgCount) {
            return (flags == OS_MESG_NOBLOCK) ? -1 : 0;
        }
        mq->first = (mq->first - 1 + mq->msgCount) % mq->msgCount;
        mq->msg[mq->first] = msg;
        mq->validCount++;
        return 0;
    }

    SDL_LockMutex(mutex);

    /* Wait for space if queue is full */
    while (mq->validCount >= mq->msgCount) {
        if (flags == OS_MESG_NOBLOCK) {
            SDL_UnlockMutex(mutex);
            return -1;
        }
        SDL_CondWait(cond_not_full, mutex);
    }

    /* Add message to FRONT of queue (jam) */
    mq->first = (mq->first - 1 + mq->msgCount) % mq->msgCount;
    mq->msg[mq->first] = msg;
    mq->validCount++;

    /* Signal that queue is no longer empty */
    SDL_CondSignal(cond_not_empty);

    SDL_UnlockMutex(mutex);
    return 0;
}

/* ============================================================================
 * Timing Implementation
 * ============================================================================ */

static u64 g_time_offset = 0;
static u64 g_perf_freq = 0;

/* N64 CPU runs at 93.75 MHz, counter increments at half that (46.875 MHz) */
#define N64_COUNTER_FREQ 46875000ULL

OSTime osGetTime(void) {
    u64 counter = SDL_GetPerformanceCounter();
    if (g_perf_freq == 0) {
        g_perf_freq = SDL_GetPerformanceFrequency();
    }
    /* Scale to N64 timing units */
    return (OSTime)((counter * N64_COUNTER_FREQ) / g_perf_freq) + g_time_offset;
}

u32 osGetCount(void) {
    return (u32)(osGetTime() & 0xFFFFFFFF);
}

void osSetTime(OSTime time) {
    OSTime current = osGetTime() - g_time_offset;
    g_time_offset = time - current;
}

/* ============================================================================
 * Timer Implementation
 *
 * Timers send messages to queues after countdown/interval expires.
 * A background thread checks active timers periodically.
 * ============================================================================ */

#define MAX_TIMERS 32

static OSTimer* g_active_timers[MAX_TIMERS];
static int g_timer_count = 0;
static SDL_mutex* g_timer_mutex = NULL;
static SDL_Thread* g_timer_thread = NULL;
static volatile int g_timer_running = 0;

static int timer_thread_func(void* data) {
    (void)data;

    while (g_timer_running) {
        OSTime now = osGetTime();

        SDL_LockMutex(g_timer_mutex);
        for (int i = 0; i < g_timer_count; i++) {
            OSTimer* t = g_active_timers[i];
            if (t && t->remaining > 0) {
                /* Check if timer expired */
                if (now >= t->remaining) {
                    /* Send message to queue */
                    if (t->mq) {
                        osSendMesg(t->mq, t->msg, OS_MESG_NOBLOCK);
                    }

                    /* Handle interval timers */
                    if (t->interval > 0) {
                        t->remaining = now + t->interval;
                    } else {
                        /* One-shot timer - remove from active list */
                        g_active_timers[i] = g_active_timers[--g_timer_count];
                        i--;
                    }
                }
            }
        }
        SDL_UnlockMutex(g_timer_mutex);

        /* Sleep briefly before checking again */
        SDL_Delay(1);
    }
    return 0;
}

void osSetTimer(OSTimer* t, OSTime countdown, OSTime interval,
                OSMesgQueue* mq, OSMesg msg) {
    t->interval = interval;
    t->remaining = osGetTime() + countdown;
    t->mq = mq;
    t->msg = msg;
    t->next = NULL;
    t->prev = NULL;

    /* Add to active timer list */
    if (g_timer_mutex) {
        SDL_LockMutex(g_timer_mutex);
        if (g_timer_count < MAX_TIMERS) {
            g_active_timers[g_timer_count++] = t;
        }
        SDL_UnlockMutex(g_timer_mutex);
    }
}

s32 osStopTimer(OSTimer* t) {
    if (g_timer_mutex) {
        SDL_LockMutex(g_timer_mutex);
        for (int i = 0; i < g_timer_count; i++) {
            if (g_active_timers[i] == t) {
                g_active_timers[i] = g_active_timers[--g_timer_count];
                break;
            }
        }
        SDL_UnlockMutex(g_timer_mutex);
    }
    t->remaining = 0;
    return 0;
}

/* ============================================================================
 * Event System
 *
 * The N64 uses hardware events (VI retrace, SI complete, etc.) to signal
 * game threads. We simulate these with a background thread that sends
 * messages at appropriate intervals.
 * ============================================================================ */

static OSMesgQueue* g_event_queues[16] = {0};
static OSMesg g_event_msgs[16] = {0};

void osSetEventMesg(s32 event, OSMesgQueue* mq, OSMesg msg) {
    if (event >= 0 && event < 16) {
        g_event_queues[event] = mq;
        g_event_msgs[event] = msg;
    }
}

/* Send a simulated event - call from main loop */
void pc_sendEvent(s32 event) {
    if (event >= 0 && event < 16 && g_event_queues[event]) {
        osSendMesg(g_event_queues[event], g_event_msgs[event], OS_MESG_NOBLOCK);
    }
}

/* ============================================================================
 * Interrupt Stubs
 * ============================================================================ */

s32 osSetIntMask(s32 mask) {
    (void)mask;
    return 0; /* Interrupts not directly used on PC */
}

/* ============================================================================
 * Controller Implementation - see input_compat.c for full SDL implementation
 * ============================================================================ */

/* NOTE: osContInit, osContStartQuery, osContStartReadData, osContGetQuery,
 * osContGetReadData are implemented in input_compat.c with full SDL support */

/* ============================================================================
 * Initialization / Shutdown
 * ============================================================================ */

void osInitialize(void) {
    /* Create thread-local storage for current thread tracking */
    g_current_thread_tls = SDL_TLSCreate();

    /* Initialize timing */
    g_perf_freq = SDL_GetPerformanceFrequency();
    g_time_offset = 0;

    /* Initialize timer system */
    g_timer_mutex = SDL_CreateMutex();
    g_timer_running = 1;
    g_timer_thread = SDL_CreateThread(timer_thread_func, "timer_thread", NULL);

    printf("OS compatibility layer initialized\n");
    printf("  Performance frequency: %llu Hz\n", (unsigned long long)g_perf_freq);
}

void __osInitialize_common(void) {
    /* Called by game code - mostly redundant with osInitialize */
}

/* Shutdown state variable (used by game code) */
int osShutdown = 0;
s32 osAppNMIBuffer[16] = {0};
u8 __osResetSwitchPressed = 0;

/* PC cleanup function - call this on exit */
void pc_osShutdown(void) {
    /* Stop timer thread */
    g_timer_running = 0;
    if (g_timer_thread) {
        SDL_WaitThread(g_timer_thread, NULL);
        g_timer_thread = NULL;
    }
    if (g_timer_mutex) {
        SDL_DestroyMutex(g_timer_mutex);
        g_timer_mutex = NULL;
    }
}
