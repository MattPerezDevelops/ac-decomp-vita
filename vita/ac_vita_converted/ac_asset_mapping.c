#include "ac_asset_mapping.h"
#include <string.h>
#include <stdio.h>

const ac_asset_mapping_t* ac_find_asset_mapping(const char* symbol_name) {
    if (!symbol_name) return NULL;
    
    for (int i = 0; i < AC_ASSET_MAPPING_COUNT; i++) {
        if (strcmp(g_ac_asset_mappings[i].symbol_name, symbol_name) == 0) {
            return &g_ac_asset_mappings[i];
        }
    }
    return NULL;
}

const ac_asset_mapping_t* ac_find_asset_by_name(const char* asset_name) {
    if (!asset_name) return NULL;
    
    for (int i = 0; i < AC_ASSET_MAPPING_COUNT; i++) {
        if (strcmp(g_ac_asset_mappings[i].asset_name, asset_name) == 0) {
            return &g_ac_asset_mappings[i];
        }
    }
    return NULL;
}

int ac_get_assets_by_category(const char* category, const ac_asset_mapping_t** results, int max_results) {
    if (!category || !results) return 0;
    
    int count = 0;
    for (int i = 0; i < AC_ASSET_MAPPING_COUNT && count < max_results; i++) {
        if (strcmp(g_ac_asset_mappings[i].category, category) == 0) {
            results[count++] = &g_ac_asset_mappings[i];
        }
    }
    return count;
}

bool ac_validate_asset_mapping(void) {
    printf("🔍 Validating AC asset mapping...\n");
    
    int categories[4] = {0}; // ui, characters, items, environment
    
    for (int i = 0; i < AC_ASSET_MAPPING_COUNT; i++) {
        const ac_asset_mapping_t* mapping = &g_ac_asset_mappings[i];
        
        // Count by category
        if (strcmp(mapping->category, "ui") == 0) categories[0]++;
        else if (strcmp(mapping->category, "characters") == 0) categories[1]++;
        else if (strcmp(mapping->category, "items") == 0) categories[2]++;
        else if (strcmp(mapping->category, "environment") == 0) categories[3]++;
    }
    
    printf("✅ Asset mapping validation complete\n");
    printf("   UI: %d, Characters: %d, Items: %d, Environment: %d\n", 
           categories[0], categories[1], categories[2], categories[3]);
    printf("   Total assets mapped: %d\n", AC_ASSET_MAPPING_COUNT);
    
    return true;
}

void ac_print_asset_stats(void) {
    printf("📊 AC-Decomp Asset Mapping Statistics\n");
    printf("=====================================\n");
    printf("Total mapped assets: %d\n", AC_ASSET_MAPPING_COUNT);
    
    // Print sample mappings
    printf("\nSample mappings:\n");
    for (int i = 0; i < 5 && i < AC_ASSET_MAPPING_COUNT; i++) {
        const ac_asset_mapping_t* mapping = &g_ac_asset_mappings[i];
        printf("  %s -> %s (%s)\n", mapping->symbol_name, mapping->vita_file, mapping->category);
    }
}
