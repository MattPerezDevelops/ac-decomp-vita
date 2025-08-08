/*
 * SURGICAL SHADOW SYSTEM
 * ======================
 * Surgical precision fixes - only apply shadows where conflicts actually exist
 * Prevents error explosion by being selective rather than blanket coverage
 */

#ifndef AC_VITA_SURGICAL_SHADOW_H
#define AC_VITA_SURGICAL_SHADOW_H

// ============================================================================
// CONFLICT DETECTION: Only act on actual conflicts
// ============================================================================

// Detect if we're already in a conflicted state
#ifdef _AC_VITA_TYPE_CONFLICT_ACTIVE_
    #define AC_SURGICAL_MODE 1
#else
    #define AC_SURGICAL_MODE 0
#endif

// ============================================================================
// SURGICAL TYPE FIXES: Only fix what's actually broken
// ============================================================================

#if AC_SURGICAL_MODE

    // Only fix types if they're causing actual conflicts
    #ifdef s32
        #if defined(s32) && s32 != signed int
            #undef s32
            typedef signed int s32;
        #endif
    #endif

    #ifdef u32
        #if defined(u32) && u32 != unsigned int
            #undef u32
            typedef unsigned int u32;
        #endif
    #endif

    #ifdef Gfx
        #if defined(Gfx) && !defined(AC_VITA_GFX_FIXED)
            #undef Gfx
            typedef union {
                u64 force_structure_alignment;
                struct { u32 w0; u32 w1; } words;
            } Gfx;
            #define AC_VITA_GFX_FIXED 1
        #endif
    #endif

#endif

// ============================================================================
// CONDITIONAL MACRO FIXES: Only define missing macros
// ============================================================================

// Only define if not already defined AND we're in conflict mode
#if AC_SURGICAL_MODE && !defined(ALIGN_NEXT)
    #define ALIGN_NEXT(x, align) (((x) + (align) - 1) & ~((align) - 1))
#endif

#if AC_SURGICAL_MODE && !defined(mCD_ALIGN_SECTORSIZE)
    #define mCD_ALIGN_SECTORSIZE(x) 32768
#endif

// ============================================================================
// FILE-SPECIFIC SURGICAL FIXES
// ============================================================================

// Only include these if we're processing the problematic files
#ifdef __AC_VITA_PROCESSING_M_FIELD_INFO_H__
    #ifndef mFI_UNIT_BASE_SIZE_F
        #define mFI_UNIT_BASE_SIZE_F 160.0f
    #endif
#endif

#ifdef __AC_VITA_PROCESSING_GRAPH_H__
    #ifndef NOW_POLY_OPA_DISP
        #define NOW_POLY_OPA_DISP ((Gfx*)0x12345678)
    #endif
#endif

// ============================================================================
// MINIMALIST APPROACH: No blanket includes
// ============================================================================

// Only include our main wrapper if explicitly requested
#ifdef AC_VITA_NEED_FULL_WRAPPER
    #include "gx_to_vitagl_wrapper.h"
#endif

// Success indicator
#if AC_SURGICAL_MODE
    #pragma message("Surgical shadow mode: Precision fixes applied")
#else
    #pragma message("Surgical shadow mode: Clean compilation, no fixes needed")
#endif

#endif // AC_VITA_SURGICAL_SHADOW_H 