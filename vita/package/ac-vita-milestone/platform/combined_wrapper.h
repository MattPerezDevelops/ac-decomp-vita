/*
 * ULTIMATE SMART COMBINED WRAPPER 
 * ===============================
 * Direct inclusion approach instead of forced compile-time inclusion
 * Avoids GCC multiple file compilation issues
 */

#ifndef AC_VITA_COMBINED_WRAPPER_H
#define AC_VITA_COMBINED_WRAPPER_H

// ============================================================================
// DIRECT SMART SHADOW INCLUSION (safer than forced inclusion)
// ============================================================================

// Include simple smart shadow directly
#include "ultimate_smart_shadow_simple.h"

// ============================================================================
// ASSET BRIDGE INTEGRATION (when ready)
// ============================================================================

#ifdef AC_VITA_ASSETS_READY
    #include "proactive_asset_bridge.h"
#endif

// Success marker
#pragma message("Combined Wrapper: Direct smart shadow inclusion - no forced compilation conflicts")

#endif // AC_VITA_COMBINED_WRAPPER_H 