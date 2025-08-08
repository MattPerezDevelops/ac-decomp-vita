// Example: Adapting AC-Decomp source files to use the Vita Asset Bridge
// This shows how to convert from compile-time #include to runtime loading

#include "ac_asset_bridge_mock.h"
#include <stdio.h>

// Example 1: Original AC-Decomp style (from req_win.c)
// ORIGINAL CODE:
/*
u16 req_win_kao_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/req_win_kao_pal.inc"
};

u8 req_win_kao_tex[] = {
#include "assets/req_win_kao_tex.inc"
};
*/

// VITA ADAPTATION:
// Instead of static arrays, use pointers that get loaded at runtime
uint16_t* req_win_kao_pal = NULL;
uint8_t* req_win_kao_tex = NULL;

// Initialization function to load assets
void req_win_init_assets(void) {
    printf("🔄 Loading req_win assets...\n");
    
    // Load palette data
    req_win_kao_pal = (uint16_t*)ac_get_palette_data("req_win_kao_pal");
    if (!req_win_kao_pal) {
        printf("❌ Failed to load req_win_kao_pal\n");
        return;
    }
    
    // Load texture data
    req_win_kao_tex = (uint8_t*)ac_get_texture_data("req_win_kao_tex");
    if (!req_win_kao_tex) {
        printf("❌ Failed to load req_win_kao_tex\n");
        return;
    }
    
    printf("✅ req_win assets loaded successfully\n");
}

// Example 2: Using the AC_ASSET_INCLUDE macro for minimal code changes
void example_macro_usage(void) {
    printf("\n🔄 Testing macro-based asset loading...\n");
    
    // This macro expands to: ac_get_asset_data("kan_win_map_tex")
    uint8_t* texture_data = AC_ASSET_INCLUDE(kan_win_map_tex);
    
    if (texture_data) {
        printf("✅ Macro loaded kan_win_map_tex at %p\n", texture_data);
        printf("   Asset size: %u bytes\n", ac_get_asset_size("kan_win_map_tex"));
    } else {
        printf("❌ Macro failed to load kan_win_map_tex\n");
    }
}

// Example 3: Preloading assets for a game module
void example_module_preload(void) {
    printf("\n🔄 Preloading UI assets for better performance...\n");
    
    // Preload all UI assets when entering a menu system
    bool success = ac_bridge_preload_category("ui");
    if (success) {
        printf("✅ UI assets preloaded\n");
    } else {
        printf("⚠️  Some UI assets failed to preload\n");
    }
    
    // Print memory usage
    printf("Cache usage: %.2f MB\n", ac_bridge_get_cache_usage() / (1024.0f * 1024.0f));
}

// Example 4: Asset reference counting for proper cleanup
void example_reference_counting(void) {
    printf("\n🔄 Testing asset reference counting...\n");
    
    // Load an asset (ref count = 1)
    uint8_t* asset1 = (uint8_t*)ac_get_asset_data("req_win_kao_tex");
    
    // Load the same asset again (ref count = 2)
    uint8_t* asset2 = (uint8_t*)ac_get_asset_data("req_win_kao_tex");
    
    printf("Asset pointers: %p, %p (should be the same)\n", asset1, asset2);
    
    // Release references
    ac_bridge_unload_symbol("req_win_kao_tex");  // ref count = 1
    ac_bridge_unload_symbol("req_win_kao_tex");  // ref count = 0, asset unloaded
    
    printf("✅ Reference counting test completed\n");
}

// Example 5: Error handling
void example_error_handling(void) {
    printf("\n🔄 Testing error handling...\n");
    
    // Try to load non-existent asset
    uint8_t* bad_asset = (uint8_t*)ac_get_asset_data("nonexistent_asset");
    if (!bad_asset) {
        ac_bridge_error_e error = ac_bridge_get_last_error();
        printf("Expected error: %s\n", ac_bridge_error_string(error));
    }
    
    // Try to load with wrong type expectation
    uint8_t* wrong_type = (uint8_t*)ac_get_palette_data("kan_win_map_tex"); // This is a texture, not palette
    if (wrong_type) {
        printf("⚠️  Type mismatch detected but data returned anyway\n");
    }
}

int main() {
    printf("🧪 AC-Decomp Source Adaptation Examples\n");
    printf("========================================\n");
    
    // Initialize the asset bridge
    if (!ac_bridge_init()) {
        printf("❌ Failed to initialize asset bridge\n");
        return 1;
    }
    
    // Run examples
    req_win_init_assets();
    example_macro_usage();
    example_module_preload();
    example_reference_counting();
    example_error_handling();
    
    // Print final statistics
    printf("\n📊 Final Asset Bridge Statistics:\n");
    ac_bridge_print_stats();
    ac_bridge_print_loaded_assets();
    
    // Cleanup
    ac_bridge_cleanup();
    
    printf("\n✅ All adaptation examples completed!\n");
    printf("\n💡 Key Integration Points for AC-Decomp:\n");
    printf("   1. Replace static asset arrays with runtime-loaded pointers\n");
    printf("   2. Add asset initialization functions for each module\n");
    printf("   3. Use AC_ASSET_INCLUDE() macro for minimal code changes\n");
    printf("   4. Preload assets by category for better performance\n");
    printf("   5. Proper cleanup with reference counting\n");
    
    return 0;
} 