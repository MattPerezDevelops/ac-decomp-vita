/**
 * @file mbi.h
 * @brief PC override for N64 Media Binary Interface header
 *
 * Routes to our PC GBI definitions
 */
#ifndef PC_MBI_H
#define PC_MBI_H

/* Include our PC GBI definitions which have Gfx, Vtx, Mtx types */
#include "pc/gbi.h"

/* Task types */
#define M_GFXTASK   1
#define M_AUDTASK   2
#define M_VIDTASK   3
#define M_HVQTASK   6
#define M_HVQMTASK  7

/* Segment macros */
#define NUM_SEGMENTS        16
#define SEGMENT_OFFSET(a)   ((unsigned int)(a) & 0x00FFFFFF)
#define SEGMENT_NUMBER(a)   (((unsigned int)(a) << 4) >> 28)
#define SEGMENT_ADDR(num, off) (((num) << 24) + (off))

#endif /* PC_MBI_H */
