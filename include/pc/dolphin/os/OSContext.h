/**
 * @file OSContext.h
 * @brief Stub OSContext for PC port
 *
 * The PC port doesn't need real CPU context - just provide
 * the typedef so structures compile.
 */
#ifndef PC_DOLPHIN_OSCONTEXT_H
#define PC_DOLPHIN_OSCONTEXT_H

#include "pc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Simplified OSContext - not used on PC */
typedef struct OSContext {
    u32 gpr[32];    /* General purpose registers (stub) */
    u32 lr;         /* Link register */
    u32 srr0;       /* Save/restore register 0 */
    u32 srr1;       /* Save/restore register 1 */
    u16 state;      /* Context state */
} OSContext;

/* Stub functions */
static inline u32 OSGetStackPointer(void) { return 0; }
static inline void OSDumpContext(OSContext *context) { (void)context; }
static inline void OSLoadContext(OSContext *context) { (void)context; }
static inline u32 OSSaveContext(OSContext *context) { (void)context; return 0; }
static inline void OSClearContext(OSContext *context) { (void)context; }
static inline OSContext *OSGetCurrentContext(void) { return NULL; }
static inline void OSSetCurrentContext(OSContext *context) { (void)context; }

#ifdef __cplusplus
}
#endif

#endif /* PC_DOLPHIN_OSCONTEXT_H */
