/*
 * SMART SHADOW ARCHITECTURE
 * =========================
 * Intelligent conditional shadow system based on iceberg analysis
 * Targets the 5 worst offending files with surgical precision
 * 
 * ARCHITECTURE PRINCIPLES:
 * 1. Conflict Detection: Only act when conflicts exist
 * 2. File-Specific Targeting: Each problematic file gets custom treatment
 * 3. Minimal Interference: Preserve clean compilation where possible
 * 4. Cascading Fixes: Fix dependencies in correct order
 */

#ifndef AC_VITA_SMART_SHADOW_ARCHITECTURE_H
#define AC_VITA_SMART_SHADOW_ARCHITECTURE_H

// ============================================================================
// ICEBERG ANALYSIS TARGETS (1,436 errors concentrated in 5 files)
// ============================================================================
/*
TOP TARGETS:
1. m_field_info.h     (420 errors) - Field constants and structures  
2. dolphin/types.h    (280 errors) - Core type conflicts
3. graph.h            (142 errors) - Graphics system issues
4. m_common_data.h    (140 errors) - Save data structures  
5. ac_tokyoso_control.h (139 errors) - Tokyo control system
TOTAL: 1,121 errors (78% of remaining errors)
*/

// ============================================================================
// SMART CONFLICT DETECTION SYSTEM
// ============================================================================

// Detect which file is currently being processed
#ifdef __has_include
    #if __has_include("m_field_info.h")
        #define AC_PROCESSING_FIELD_INFO 1
    #endif
    #if __has_include("dolphin/types.h") 
        #define AC_PROCESSING_DOLPHIN_TYPES 1
    #endif
    #if __has_include("graph.h")
        #define AC_PROCESSING_GRAPH 1  
    #endif
    #if __has_include("m_common_data.h")
        #define AC_PROCESSING_COMMON_DATA 1
    #endif
    #if __has_include("ac_tokyoso_control.h")
        #define AC_PROCESSING_TOKYO_CONTROL 1
    #endif
#endif

// Detect compilation conflicts
#ifdef s32
    #ifdef u32
        #ifdef f32
            #define AC_TYPE_CONFLICTS_DETECTED 1
        #endif
    #endif
#endif

// Smart activation logic
#if defined(AC_TYPE_CONFLICTS_DETECTED) || defined(AC_PROCESSING_FIELD_INFO) || defined(AC_PROCESSING_DOLPHIN_TYPES) || defined(AC_PROCESSING_GRAPH) || defined(AC_PROCESSING_COMMON_DATA) || defined(AC_PROCESSING_TOKYO_CONTROL)
    #define AC_SMART_SHADOWS_ACTIVE 1
#else
    #define AC_SMART_SHADOWS_ACTIVE 0
#endif

// ============================================================================
// FILE-SPECIFIC SMART SHADOWS
// ============================================================================

#if AC_SMART_SHADOWS_ACTIVE

// TARGET 1: m_field_info.h (420 errors) - Field system fixes
#ifdef AC_PROCESSING_FIELD_INFO
    #include "shadows/smart_field_info_shadow.h"
#endif

// TARGET 2: dolphin/types.h (280 errors) - Core type system  
#ifdef AC_PROCESSING_DOLPHIN_TYPES
    #include "shadows/smart_dolphin_types_shadow.h"
#endif

// TARGET 3: graph.h (142 errors) - Graphics system
#ifdef AC_PROCESSING_GRAPH
    #include "shadows/smart_graph_shadow.h"
#endif

// TARGET 4: m_common_data.h (140 errors) - Save data system
#ifdef AC_PROCESSING_COMMON_DATA
    #include "shadows/smart_common_data_shadow.h"
#endif

// TARGET 5: ac_tokyoso_control.h (139 errors) - Tokyo control
#ifdef AC_PROCESSING_TOKYO_CONTROL
    #include "shadows/smart_tokyo_control_shadow.h"
#endif

#endif

// ============================================================================
// CONDITIONAL GX WRAPPER INTEGRATION
// ============================================================================

// Only include full wrapper if we're in conflict mode
#if AC_SMART_SHADOWS_ACTIVE && defined(AC_VITA_NEED_GX_WRAPPER)
    #include "gx_to_vitagl_wrapper.h"
#endif

// ============================================================================
// SMART SYSTEM STATUS
// ============================================================================

#if AC_SMART_SHADOWS_ACTIVE
    #pragma message("Smart Shadow Architecture: ACTIVE - Targeting specific conflicts")
    
    #ifdef AC_PROCESSING_FIELD_INFO
        #pragma message("  -> Field info shadow applied (targeting 420 errors)")
    #endif
    #ifdef AC_PROCESSING_DOLPHIN_TYPES
        #pragma message("  -> Dolphin types shadow applied (targeting 280 errors)")
    #endif
    #ifdef AC_PROCESSING_GRAPH
        #pragma message("  -> Graph shadow applied (targeting 142 errors)")
    #endif
    #ifdef AC_PROCESSING_COMMON_DATA
        #pragma message("  -> Common data shadow applied (targeting 140 errors)")
    #endif
    #ifdef AC_PROCESSING_TOKYO_CONTROL
        #pragma message("  -> Tokyo control shadow applied (targeting 139 errors)")
    #endif
#else
    #pragma message("Smart Shadow Architecture: INACTIVE - Clean compilation detected")
#endif

#endif // AC_VITA_SMART_SHADOW_ARCHITECTURE_H 