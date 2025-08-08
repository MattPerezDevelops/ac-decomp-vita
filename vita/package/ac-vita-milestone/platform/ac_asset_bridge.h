/*
 * Animal Crossing Asset Bridge - Preprocessor Interception
 * =========================================================
 * Intercepts asset includes from ORIGINAL AC-decomp source without modifications
 * Redirects to runtime loading from external Vita folder: ux0:data/AnimalCrossing/assets/
 */

#ifndef AC_ASSET_BRIDGE_H
#define AC_ASSET_BRIDGE_H

#include "ac_runtime_asset_loader.h"

// Asset bridge system - preprocessor interception for pristine AC source
#ifdef AC_USE_ASSET_BRIDGE

// ===================================================================
// TYPE DEFINITIONS FOR RUNTIME BRIDGE
// ===================================================================

// Runtime asset data type
typedef struct {
    const void* data;
    size_t size;
    char name[64];
} runtime_asset_t;

// ===================================================================
// PREPROCESSOR ASSET INTERCEPTION STRATEGY
// ===================================================================
// The original AC-decomp source has patterns like:
//   u16 assetName[] = { #include "assets/file.inc" };
//
// We intercept this by redefining the include path to point to our data
// This preserves the original source code completely unchanged!
// ===================================================================

// Universal asset data for all missing includes
extern const unsigned char universal_asset_placeholder[];

// Asset data initialization function
extern void ac_asset_bridge_init_runtime_data(void);

// ===================================================================
// ASSET INCLUDE REDIRECTION MACROS
// ===================================================================
// These macros redirect specific asset includes to runtime data

// Strategy: Create minimal placeholder data that compiles
// The actual assets will be loaded at runtime from external folder

// Common asset includes that appear in original AC-decomp
#ifndef ASSET_INCLUDE_INTERCEPTED
#define ASSET_INCLUDE_INTERCEPTED

// Redirect asset includes to placeholder data
#define aKOI_obj_e_koinobori_a_pal_inc_content 0x00, 0x00
#define aLOT_obj_01_lotus_pal_inc_content 0x00, 0x00  
#define obj_e_koinobori_b_pal_inc_content 0x00, 0x00
#define br_shop_pal_inc_content 0x00, 0x00
#define br_shop_winter_pal_inc_content 0x00, 0x00

#endif // ASSET_INCLUDE_INTERCEPTED

// Function to initialize runtime asset bridge with external folder
extern int ac_asset_bridge_init(void);
extern void ac_asset_bridge_cleanup(void);

// Function to get runtime asset from external folder  
extern const void* ac_get_runtime_asset(const char* asset_name);

#endif // AC_USE_ASSET_BRIDGE

#endif // AC_ASSET_BRIDGE_H 