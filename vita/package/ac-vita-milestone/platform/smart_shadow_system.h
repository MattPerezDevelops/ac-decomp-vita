/*
 * SMART SHADOW HEADER SYSTEM
 * ==========================
 * Intelligent shadow headers that only apply when conflicts are detected
 * This prevents the "error iceberg" by handling conflicts selectively
 */

#ifndef AC_VITA_SMART_SHADOW_SYSTEM_H
#define AC_VITA_SMART_SHADOW_SYSTEM_H

// ============================================================================
// SMART CONFLICT DETECTION SYSTEM
// ============================================================================

// Check if we're in a conflicting context
#ifndef AC_VITA_CONFLICT_DETECTED
    // Test for known conflict indicators
    #ifdef _conflicting_types_detected_
        #define AC_VITA_CONFLICT_DETECTED 1
    #elif defined(__GNUC__) && !defined(AC_VITA_CLEAN_COMPILE)
        // GCC compilation with potential conflicts
        #define AC_VITA_CONFLICT_DETECTED 1
    #else
        #define AC_VITA_CONFLICT_DETECTED 0
    #endif
#endif

// ============================================================================
// CONDITIONAL SHADOW HEADERS
// ============================================================================

#if AC_VITA_CONFLICT_DETECTED

    // Only include our wrapper when conflicts are detected
    #include "gx_to_vitagl_wrapper.h"
    
    // Mark that we've applied shadows
    #ifndef AC_VITA_SHADOWS_APPLIED
    #define AC_VITA_SHADOWS_APPLIED 1
    #endif
    
    // Override problematic type definitions
    #ifdef s32
        #undef s32
    #endif
    #ifdef u32
        #undef u32
    #endif
    #ifdef f32
        #undef f32
    #endif
    
    // Redefine with our clean versions
    typedef signed int s32;
    typedef unsigned int u32; 
    typedef float f32;
    
    // Override problematic structure definitions
    #ifdef Gfx
        #undef Gfx
    #endif
    
    // Use our hybrid Gfx definition
    typedef union {
        u64 force_structure_alignment;
        struct {
            u32 w0;
            u32 w1;
        } words;
    } Gfx;

#else
    // Clean compilation - let original headers work normally
    #ifndef AC_VITA_CLEAN_COMPILE
    #define AC_VITA_CLEAN_COMPILE 1
    #endif
#endif

// ============================================================================
// SMART MACRO SYSTEM
// ============================================================================

// Only define macros if they're causing conflicts
#ifndef ALIGN_NEXT
    #if AC_VITA_CONFLICT_DETECTED
        #define ALIGN_NEXT(x, align) (((x) + (align) - 1) & ~((align) - 1))
    #endif
#endif

#ifndef mCD_ALIGN_SECTORSIZE
    #if AC_VITA_CONFLICT_DETECTED
        #define mCD_ALIGN_SECTORSIZE(x) 32768
    #endif
#endif

// ============================================================================
// CONFLICT RESOLUTION STATISTICS
// ============================================================================

#if AC_VITA_CONFLICT_DETECTED
    #pragma message("Smart shadow system activated - conflicts detected and resolved")
#else
    #pragma message("Clean compilation detected - original headers used")
#endif

#endif // AC_VITA_SMART_SHADOW_SYSTEM_H 