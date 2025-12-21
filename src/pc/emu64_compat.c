/**
 * @file emu64_compat.c
 * @brief emu64 compatibility layer for PC port
 *
 * This provides the same interface as the GameCube's emu64 system,
 * but routes display list execution to our GBI interpreter.
 */

#include <stdio.h>
#include "pc/platform.h"  /* For u8, u32, etc. */
#include "pc/gbi.h"  /* GBI types and commands */

/* From emu64_wrapper.h interface */
u8 FrameCansel = 0;

/**
 * Set ucode info - no-op on PC (we don't use microcode)
 */
void emu64_set_ucode_info(int count, void* ucode_info) {
    (void)count;
    (void)ucode_info;
    /* No-op - PC doesn't use N64 microcode */
}

/**
 * Set first ucode - no-op on PC
 */
void emu64_set_first_ucode(void* ucode) {
    (void)ucode;
    /* No-op - PC doesn't use N64 microcode */
}

/**
 * Execute a display list
 * This is the core function - it calls our GBI interpreter
 */
extern void gbi_set_heap_base(void* base);

static int dl_count = 0;
void emu64_taskstart(Gfx* gfx) {
    printf("[EMU64] emu64_taskstart entry, gfx=%p\n", (void*)gfx); fflush(stdout);
    if (gfx) {
        printf("[EMU64] gfx is not NULL\n"); fflush(stdout);
        /* Set heap base for 64-bit pointer recovery */
        gbi_set_heap_base(gfx);
        printf("[EMU64] heap base set\n"); fflush(stdout);

        dl_count++;
        printf("[EMU64] dl_count=%d\n", dl_count); fflush(stdout);
        if (dl_count <= 10) {
            printf("[EMU64] emu64_taskstart: executing display list %d at %p\n", dl_count, (void*)gfx);
            fflush(stdout);
            printf("[EMU64]   heap_base high bits: 0x%lX\n", (unsigned long)(((uintptr_t)gfx) & 0xFFFFFFFF00000000ULL));
            fflush(stdout);
            /* Dump first few commands to debug */
            for (int i = 0; i < 5; i++) {
                u32 w0 = gfx[i].words.w0;
                u32 w1 = gfx[i].words.w1;
                u8 opcode = (w0 >> 24) & 0xFF;
                printf("[EMU64]   cmd[%d]: opcode=0x%02X w0=0x%08X w1=0x%08X", i, opcode, w0, w1);
                /* If it looks like a G_DL, show recovered pointer */
                if (opcode == 0x06 || opcode == 0xDE) {
                    uintptr_t recovered = (((uintptr_t)gfx) & 0xFFFFFFFF00000000ULL) | (uintptr_t)w1;
                    printf(" -> recovered: %p", (void*)recovered);
                }
                printf("\n");
                fflush(stdout);
            }
        }
        printf("[EMU64] Calling gbi_execute\n"); fflush(stdout);
        gbi_execute(gfx);
        printf("[EMU64] gbi_execute returned\n"); fflush(stdout);
    }
}

/**
 * Initialize emu64 - prepares for rendering
 */
void emu64_init(void) {
    /* GBI interpreter is already initialized in main */
    /* Could reset per-frame state here if needed */
}

/**
 * Refresh emu64 state
 */
void emu64_refresh(void) {
    /* No-op for now */
}

/**
 * Cleanup after rendering
 */
void emu64_cleanup(void) {
    /* No-op for now */
}

/**
 * Set texture cache data entry
 */
void emu64_texture_cache_data_entry_set(void* begin, void* end) {
    (void)begin;
    (void)end;
    /* TODO: Could use this for texture management */
}
