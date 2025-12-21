#ifndef PTR_REGISTRY_H
#define PTR_REGISTRY_H

#include "dolphin/types.h"

/**
 * Pointer Registry - Maps truncated 32-bit addresses to full 64-bit pointers
 *
 * On 64-bit systems, pointers stored in 32-bit fields (like Gfx.words.w1)
 * get truncated. This registry allows recovery of the full pointer by
 * looking up the truncated address.
 *
 * Usage:
 *   1. Call ptr_registry_init() at startup
 *   2. Register pointers with ptr_registry_add()
 *   3. Look up truncated addresses with ptr_registry_lookup()
 */

/* Initialize the pointer registry */
void ptr_registry_init(void);

/* Register a pointer for later lookup
 * @param ptr       Full 64-bit pointer to register
 * @param name      Debug name for logging (can be NULL)
 */
void ptr_registry_add(void* ptr, const char* name);

/* Look up a truncated address and return the full pointer
 * @param truncated Lower 32 bits of the original pointer
 * @return          Full 64-bit pointer, or NULL if not found
 */
void* ptr_registry_lookup(u32 truncated);

/* Get statistics about the registry */
int ptr_registry_count(void);

/* Debug: print all registered pointers */
void ptr_registry_dump(void);

/**
 * Register a buffer range for range-based lookup.
 * When looking up an address that falls within this range,
 * the correct full pointer will be computed.
 *
 * @param base      Base address of the buffer
 * @param size      Size of the buffer in bytes
 * @param name      Debug name for logging
 */
void ptr_registry_add_range(void* base, size_t size, const char* name);

#endif /* PTR_REGISTRY_H */
