/**
 * @file mem_compat.c
 * @brief Memory allocation compatibility layer
 *
 * Part of Layer 2 (Porting Abstraction Layer).
 * Simple wrapper around standard malloc/free - PC has unlimited memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc/platform.h"

/* Memory tracking (debug) */
#ifdef DEBUG
static size_t g_total_allocated = 0;
static size_t g_allocation_count = 0;
#endif

/**
 * Allocate memory
 */
void* osAlloc(size_t size) {
    void* ptr = malloc(size);

#ifdef DEBUG
    if (ptr) {
        g_total_allocated += size;
        g_allocation_count++;
    }
#endif

    return ptr;
}

/**
 * Free memory
 */
void osFree(void* ptr) {
    if (ptr) {
#ifdef DEBUG
        g_allocation_count--;
#endif
        free(ptr);
    }
}

/**
 * Allocate aligned memory
 */
void* osAllocAligned(size_t alignment, size_t size) {
    void* ptr = NULL;

#if defined(PLATFORM_WINDOWS)
    ptr = _aligned_malloc(size, alignment);
#else
    if (posix_memalign(&ptr, alignment, size) != 0) {
        ptr = NULL;
    }
#endif

#ifdef DEBUG
    if (ptr) {
        g_total_allocated += size;
        g_allocation_count++;
    }
#endif

    return ptr;
}

/**
 * Free aligned memory
 */
void osFreeAligned(void* ptr) {
    if (ptr) {
#ifdef DEBUG
        g_allocation_count--;
#endif

#if defined(PLATFORM_WINDOWS)
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }
}

/**
 * Reallocate memory
 */
void* osRealloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}

/**
 * Allocate and zero memory
 */
void* osCalloc(size_t count, size_t size) {
    void* ptr = calloc(count, size);

#ifdef DEBUG
    if (ptr) {
        g_total_allocated += count * size;
        g_allocation_count++;
    }
#endif

    return ptr;
}

/**
 * Get memory stats (debug)
 */
void osMemGetStats(size_t* total_allocated, size_t* allocation_count) {
#ifdef DEBUG
    if (total_allocated) *total_allocated = g_total_allocated;
    if (allocation_count) *allocation_count = g_allocation_count;
#else
    if (total_allocated) *total_allocated = 0;
    if (allocation_count) *allocation_count = 0;
#endif
}

/**
 * Print memory stats (debug)
 */
void osMemPrintStats(void) {
#ifdef DEBUG
    printf("Memory Stats:\n");
    printf("  Total allocated: %zu bytes\n", g_total_allocated);
    printf("  Active allocations: %zu\n", g_allocation_count);
#endif
}
