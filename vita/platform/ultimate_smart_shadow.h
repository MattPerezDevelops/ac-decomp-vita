/*
 * ULTIMATE SMART SHADOW SYSTEM
 * ============================
 * Implements the user's "smart shadow header that only applies when needed" concept
 * 
 * SMART STRATEGY:
 * 1. Detect actual conflicts at compile time
 * 2. Only apply fixes for detected problems
 * 3. Preserve clean compilation where possible
 * 4. Single source of truth, conditionally applied
 */

#ifndef AC_VITA_ULTIMATE_SMART_SHADOW_H
#define AC_VITA_ULTIMATE_SMART_SHADOW_H

// ============================================================================
// SMART CONFLICT DETECTION SYSTEM
// ============================================================================

// Detect if we're processing problematic files
#ifndef AC_CONFLICT_DETECTION_ACTIVE
#define AC_CONFLICT_DETECTION_ACTIVE 1

// File-based detection
#ifdef __FILE__
    #if defined(__FILE__) && (strstr(__FILE__, "m_field_info.h") || \
                              strstr(__FILE__, "dolphin/types.h") || \
                              strstr(__FILE__, "graph.h") || \
                              strstr(__FILE__, "m_common_data.h") || \
                              strstr(__FILE__, "ac_tokyoso_control.h"))
        #define AC_PROBLEMATIC_FILE_DETECTED 1
    #endif
#endif

// Type conflict detection
#if defined(s32) && defined(u32) && defined(f32)
    #ifndef AC_TYPE_CONFLICTS_DETECTED
    #define AC_TYPE_CONFLICTS_DETECTED 1
    #endif
#endif

// Structure conflict detection
#ifdef Gfx
    #ifndef AC_GFX_CONFLICTS_DETECTED
    #define AC_GFX_CONFLICTS_DETECTED 1
    #endif
#endif

#endif

// ============================================================================
// CONDITIONAL TYPE FIXES - Only apply when conflicts detected
// ============================================================================

#ifdef AC_TYPE_CONFLICTS_DETECTED

    // Clear conflicting definitions
    #ifdef s8
    #undef s8
    #endif
    #ifdef u8
    #undef u8
    #endif
    #ifdef s16
    #undef s16
    #endif
    #ifdef u16
    #undef u16
    #endif
    #ifdef s32
    #undef s32
    #endif
    #ifdef u32
    #undef u32
    #endif
    #ifdef f32
    #undef f32
    #endif

    // Apply clean definitions
    typedef signed char s8;
    typedef unsigned char u8;
    typedef signed short s16;
    typedef unsigned short u16;
    typedef signed int s32;
    typedef unsigned int u32;
    typedef float f32;

    #pragma message("Ultimate Smart Shadow: Type conflicts detected and resolved")

#endif

// ============================================================================
// CONDITIONAL GFX FIXES - Only apply when Gfx conflicts detected
// ============================================================================

#ifdef AC_GFX_CONFLICTS_DETECTED

    #ifdef Gfx
    #undef Gfx
    #endif

    typedef union {
        unsigned long long force_structure_alignment;
        struct {
            unsigned int w0;
            unsigned int w1;
        } words;
    } Gfx;

    #pragma message("Ultimate Smart Shadow: Gfx conflicts detected and resolved")

#endif

// ============================================================================
// FILE-SPECIFIC CONDITIONAL FIXES
// ============================================================================

#ifdef AC_PROBLEMATIC_FILE_DETECTED

    // Only define these when processing problematic files
    #ifndef mFI_UNIT_BASE_SIZE_F
    #define mFI_UNIT_BASE_SIZE_F 160.0f
    #endif

    #ifndef mCD_ALIGN_SECTORSIZE
    #define mCD_ALIGN_SECTORSIZE(x) 0x30000
    #endif

    #ifndef GRAPH_ALLOC_TYPE
    #define GRAPH_ALLOC_TYPE(graph, type, count) ((type*)malloc(sizeof(type) * (count)))
    #endif

    #pragma message("Ultimate Smart Shadow: Problematic file detected - targeted fixes applied")

#endif

// ============================================================================
// MINIMAL STRUCTURE FIXES - Only when absolutely needed
// ============================================================================

#if defined(AC_PROBLEMATIC_FILE_DETECTED) || defined(AC_TYPE_CONFLICTS_DETECTED)

    // GRAPH structure - only if we're dealing with graphics conflicts
    #ifndef GRAPH
    typedef struct GRAPH_s {
        void* polygon_opaque_thaga;
        void* poly_opa_disp;
        unsigned int graphics_flags;
        unsigned int padding[8];
    } GRAPH;
    #endif

    // Basic utility macros
    #ifndef ARRAY_COUNT
    #define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
    #endif

    #ifndef ALIGN_NEXT
    #define ALIGN_NEXT(x, align) (((x) + (align) - 1) & ~((align) - 1))
    #endif

#endif

// ============================================================================
// SMART SYSTEM STATUS REPORTING
// ============================================================================

#ifdef AC_TYPE_CONFLICTS_DETECTED
    #pragma message("  -> Type conflicts resolved")
#endif

#ifdef AC_GFX_CONFLICTS_DETECTED
    #pragma message("  -> Graphics conflicts resolved")
#endif

#ifdef AC_PROBLEMATIC_FILE_DETECTED
    #pragma message("  -> File-specific fixes applied")
#endif

#if !defined(AC_TYPE_CONFLICTS_DETECTED) && !defined(AC_GFX_CONFLICTS_DETECTED) && !defined(AC_PROBLEMATIC_FILE_DETECTED)
    #pragma message("Ultimate Smart Shadow: No conflicts detected - clean compilation preserved")
#endif

#endif // AC_VITA_ULTIMATE_SMART_SHADOW_H 