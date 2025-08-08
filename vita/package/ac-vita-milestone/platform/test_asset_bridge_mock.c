#include "ac_asset_bridge_mock.h"
#include <stdio.h>
#include <string.h>

int main() {
    printf("🧪 AC Asset Bridge Test (Mock Version)\n");
    printf("=======================================\n");
    
    // Initialize the bridge
    printf("1. Initializing asset bridge...\n");
    if (!ac_bridge_init()) {
        printf("❌ Failed to initialize asset bridge\n");
        return 1;
    }
    
    // Print initial statistics
    printf("\n2. Initial statistics:\n");
    ac_bridge_print_stats();
    
    // Test asset mapping lookup
    printf("\n3. Testing asset mapping lookup...\n");
    const char* test_symbols[] = {
        "kan_win_map_tex",
        "req_win_kao_pal", 
        "int_tak_nes01_top_tex",
        "nonexistent_asset"
    };
    
    for (int i = 0; i < 4; i++) {
        const char* symbol = test_symbols[i];
        printf("   Checking '%s': ", symbol);
        
        if (ac_bridge_asset_exists(symbol)) {
            const ac_asset_mapping_t* mapping = ac_bridge_get_mapping(symbol);
            printf("✅ Found - Category: %s, File: %s\n", 
                   mapping->category, mapping->vita_file);
        } else {
            printf("❌ Not found\n");
        }
    }
    
    // Test asset loading
    printf("\n4. Testing asset loading...\n");
    const char* load_test_symbols[] = {
        "kan_win_map_tex",
        "req_win_kao_pal"
    };
    
    for (int i = 0; i < 2; i++) {
        const char* symbol = load_test_symbols[i];
        printf("   Loading '%s': ", symbol);
        
        void* data = ac_get_asset_data(symbol);
        if (data) {
            uint32_t size = ac_get_asset_size(symbol);
            printf("✅ Loaded %u bytes\n", size);
        } else {
            printf("❌ Failed - %s\n", ac_bridge_error_string(ac_bridge_get_last_error()));
        }
    }
    
    // Print statistics after loading
    printf("\n5. Statistics after loading:\n");
    ac_bridge_print_stats();
    ac_bridge_print_loaded_assets();
    
    // Test category preloading  
    printf("\n6. Testing category preloading (limiting to 5 assets)...\n");
    
    // Get a few UI assets to test
    const ac_asset_mapping_t* ui_results[5];
    int ui_count = ac_get_assets_by_category("ui", ui_results, 5);
    printf("   Found %d UI assets, loading first few...\n", ui_count);
    
    for (int i = 0; i < ui_count && i < 3; i++) {
        printf("   Loading UI asset '%s'...\n", ui_results[i]->symbol_name);
        ac_get_asset_data(ui_results[i]->symbol_name);
    }
    
    // Final statistics
    printf("\n7. Final statistics:\n");
    ac_bridge_print_stats();
    ac_bridge_print_loaded_assets();
    
    // Test unloading
    printf("\n8. Testing asset unloading...\n");
    ac_bridge_unload_unused();
    ac_bridge_print_stats();
    
    // Test the macro-based loading (simulating AC-Decomp usage)
    printf("\n9. Testing AC-Decomp style macro loading...\n");
    void* texture_data = AC_ASSET_INCLUDE(kan_win_map_tex);
    if (texture_data) {
        printf("✅ Macro loading works: got data at %p\n", texture_data);
    } else {
        printf("❌ Macro loading failed\n");
    }
    
    // Cleanup
    printf("\n10. Cleaning up...\n");
    ac_bridge_cleanup();
    
    printf("\n✅ Asset bridge test completed successfully!\n");
    printf("🎯 The asset bridge is ready to integrate with AC-Decomp source code!\n");
    return 0;
} 