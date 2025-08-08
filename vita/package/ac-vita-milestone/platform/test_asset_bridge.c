#include "ac_asset_bridge.h"
#include <stdio.h>
#include <string.h>

int main() {
    printf("🧪 AC Asset Bridge Test\n");
    printf("========================\n");
    
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
    printf("\n6. Testing category preloading...\n");
    ac_bridge_preload_category("ui");
    
    // Final statistics
    printf("\n7. Final statistics:\n");
    ac_bridge_print_stats();
    
    // Test unloading
    printf("\n8. Testing asset unloading...\n");
    ac_bridge_unload_unused();
    ac_bridge_print_stats();
    
    // Cleanup
    printf("\n9. Cleaning up...\n");
    ac_bridge_cleanup();
    
    printf("\n✅ Asset bridge test completed!\n");
    return 0;
} 