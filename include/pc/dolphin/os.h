/**
 * @file os.h
 * @brief PC override for dolphin/os.h
 *
 * Provides N64-style OS API stubs
 */
#ifndef PC_DOLPHIN_OS_H
#define PC_DOLPHIN_OS_H

#include "pc/types.h"
#include "pc/os_compat.h"

/* Time functions */
u64 OSGetTime(void);

/* Memory functions */
void* OSAllocFromHeap(int heap, size_t size);
void OSFreeToHeap(int heap, void* ptr);

/* Thread priority */
typedef s32 OSPri;

/* bzero/bcopy - standard memory functions */
#include <string.h>
#define bzero(ptr, size) memset((ptr), 0, (size))
#define bcopy(src, dst, size) memmove((dst), (src), (size))

#endif /* PC_DOLPHIN_OS_H */
