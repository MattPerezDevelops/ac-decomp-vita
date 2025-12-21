/**
 * @file dsp.h
 * @brief Stub DSP for PC port
 *
 * The PC port doesn't need GameCube DSP - just provide
 * the typedefs so structures compile.
 */
#ifndef PC_DOLPHIN_DSP_H
#define PC_DOLPHIN_DSP_H

#include "pc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DSP_TASK_FLAG_CLEARALL 0x00000000
#define DSP_TASK_FLAG_ATTACHED 0x00000001
#define DSP_TASK_FLAG_CANCEL   0x00000002

#define DSP_TASK_STATE_INIT  0
#define DSP_TASK_STATE_RUN   1
#define DSP_TASK_STATE_YIELD 2
#define DSP_TASK_STATE_DONE  3

typedef void (*DSPCallback)(void* task);

/* Simplified DSPTaskInfo - not used on PC */
typedef struct STRUCT_DSP_TASK {
    u32 state;
    u32 priority;
    u32 flags;
    u16* iram_mmem_addr;
    u32 iram_length;
    u32 iram_addr;
    u16* dram_mmem_addr;
    u32 dram_length;
    u32 dram_addr;
    u16 dsp_init_vector;
    u16 dsp_resume_vector;
    DSPCallback init_cb;
    DSPCallback res_cb;
    DSPCallback done_cb;
    DSPCallback req_cb;
    struct STRUCT_DSP_TASK* next;
    struct STRUCT_DSP_TASK* prev;
    u64 t_context;
    u64 t_task;
} DSPTaskInfo;

/* Stub functions */
static inline void DSPInit(void) { }
static inline void DSPReset(void) { }
static inline void DSPHalt(void) { }
static inline void DSPSendMailToDSP(u32 mail) { (void)mail; }
static inline u32 DSPCheckMailToDSP(void) { return 0; }
static inline u32 DSPCheckMailFromDSP(void) { return 0; }
static inline u32 DSPReadMailFromDSP(void) { return 0; }
static inline u32 DSPGetDMAStatus(void) { return 0; }
static inline DSPTaskInfo* DSPAddTask(DSPTaskInfo* task) { (void)task; return NULL; }

#ifdef __cplusplus
}
#endif

#endif /* PC_DOLPHIN_DSP_H */
