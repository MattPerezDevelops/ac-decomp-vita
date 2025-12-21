#include "ptr_registry.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/**
 * Pointer Registry Implementation
 *
 * Uses a hash table with linear probing for O(1) average lookup.
 * The hash key is the lower 32 bits of the pointer (the truncated address).
 *
 * Also supports range-based registration for dynamic buffers.
 */

#define REGISTRY_SIZE 16384  /* Power of 2 for fast modulo */
#define REGISTRY_MASK (REGISTRY_SIZE - 1)

/* Maximum number of buffer ranges we can track */
#define MAX_RANGES 32

typedef struct {
    u32 truncated;      /* Lower 32 bits of pointer (hash key) */
    void* full;         /* Full 64-bit pointer */
    const char* name;   /* Debug name */
    int valid;          /* Entry is occupied */
} PtrEntry;

/* Range entry for buffer-based lookups */
typedef struct {
    void* base;         /* Base address of buffer */
    u32 trunc_base;     /* Truncated base address */
    u32 size;           /* Size of buffer */
    const char* name;   /* Debug name */
    int valid;          /* Entry is occupied */
} RangeEntry;

static PtrEntry g_registry[REGISTRY_SIZE];
static RangeEntry g_ranges[MAX_RANGES];
static int g_registry_count = 0;
static int g_range_count = 0;
static int g_registry_initialized = 0;

void ptr_registry_init(void) {
    memset(g_registry, 0, sizeof(g_registry));
    memset(g_ranges, 0, sizeof(g_ranges));
    g_registry_count = 0;
    g_range_count = 0;
    g_registry_initialized = 1;
    printf("[PTR_REG] Pointer registry initialized (capacity=%d, ranges=%d)\n",
           REGISTRY_SIZE, MAX_RANGES);
}

void ptr_registry_add(void* ptr, const char* name) {
    if (!g_registry_initialized) {
        ptr_registry_init();
    }

    if (ptr == NULL) {
        return;
    }

    u32 truncated = (u32)(uintptr_t)ptr;
    u32 index = truncated & REGISTRY_MASK;

    /* Linear probing to find empty slot or existing entry */
    for (int i = 0; i < REGISTRY_SIZE; i++) {
        u32 probe = (index + i) & REGISTRY_MASK;
        PtrEntry* entry = &g_registry[probe];

        if (!entry->valid) {
            /* Empty slot - insert new entry */
            entry->truncated = truncated;
            entry->full = ptr;
            entry->name = name;
            entry->valid = 1;
            g_registry_count++;

#if 1  /* Verbose logging - enable for debugging */
            printf("[PTR_REG] Added %s: 0x%08X -> %p\n",
                   name ? name : "?", truncated, ptr);
#endif
            return;
        }

        if (entry->truncated == truncated) {
            /* Already registered - verify it's the same pointer */
            if (entry->full != ptr) {
                printf("[PTR_REG] WARNING: Hash collision! 0x%08X maps to both %p and %p\n",
                       truncated, entry->full, ptr);
            }
            return;
        }
    }

    printf("[PTR_REG] ERROR: Registry full! Cannot add %s (0x%08X)\n",
           name ? name : "?", truncated);
}

void ptr_registry_add_range(void* base, size_t size, const char* name) {
    if (!g_registry_initialized) {
        ptr_registry_init();
    }

    if (base == NULL || size == 0) {
        return;
    }

    if (g_range_count >= MAX_RANGES) {
        printf("[PTR_REG] ERROR: Range table full! Cannot add %s\n",
               name ? name : "?");
        return;
    }

    u32 trunc_base = (u32)(uintptr_t)base;

    /* Find empty slot */
    for (int i = 0; i < MAX_RANGES; i++) {
        if (!g_ranges[i].valid) {
            g_ranges[i].base = base;
            g_ranges[i].trunc_base = trunc_base;
            g_ranges[i].size = (u32)size;
            g_ranges[i].name = name;
            g_ranges[i].valid = 1;
            g_range_count++;

            printf("[PTR_REG] Added range %s: 0x%08X-0x%08X -> %p (%zu bytes)\n",
                   name ? name : "?", trunc_base, trunc_base + (u32)size,
                   base, size);
            return;
        }
    }
}

void* ptr_registry_lookup(u32 truncated) {
    if (!g_registry_initialized || truncated == 0) {
        return NULL;
    }

    u32 index = truncated & REGISTRY_MASK;

    /* First, try exact match in hash table */
    for (int i = 0; i < REGISTRY_SIZE; i++) {
        u32 probe = (index + i) & REGISTRY_MASK;
        PtrEntry* entry = &g_registry[probe];

        if (!entry->valid) {
            /* Empty slot - not found in hash table, try ranges */
            break;
        }

        if (entry->truncated == truncated) {
            /* Found exact match */
            static int lookup_debug = 0;
            if (lookup_debug < 20) {
                printf("[PTR_REG] FOUND exact 0x%08X -> %p (%s)\n",
                       truncated, entry->full, entry->name ? entry->name : "?");
                lookup_debug++;
            }
            return entry->full;
        }
    }

    /* Not found in hash table - check if address falls within a registered range */
    for (int i = 0; i < MAX_RANGES; i++) {
        RangeEntry* range = &g_ranges[i];
        if (!range->valid) continue;

        /* Check if truncated address falls within this range */
        if (truncated >= range->trunc_base &&
            truncated < range->trunc_base + range->size) {
            /* Compute offset and return full pointer */
            u32 offset = truncated - range->trunc_base;
            void* result = (void*)((uintptr_t)range->base + offset);

            static int range_debug = 0;
            if (range_debug < 50) {
                printf("[PTR_REG] FOUND in range %s: 0x%08X -> %p (offset=0x%X)\n",
                       range->name ? range->name : "?", truncated, result, offset);
                range_debug++;
            }
            return result;
        }
    }

    return NULL;
}

int ptr_registry_count(void) {
    return g_registry_count;
}

void ptr_registry_dump(void) {
    printf("[PTR_REG] === Registry Dump (%d entries) ===\n", g_registry_count);

    int printed = 0;
    for (int i = 0; i < REGISTRY_SIZE && printed < 50; i++) {
        PtrEntry* entry = &g_registry[i];
        if (entry->valid) {
            printf("  [%4d] 0x%08X -> %p (%s)\n",
                   i, entry->truncated, entry->full,
                   entry->name ? entry->name : "?");
            printed++;
        }
    }

    if (g_registry_count > 50) {
        printf("  ... and %d more entries\n", g_registry_count - 50);
    }
}
