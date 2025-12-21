/**
 * @file tha_compat.c
 * @brief TwoHeadArena (THA) and THA_GA implementations for PC port
 *
 * These are simple arena allocators used for display list memory management.
 * The original game uses these to manage display list buffers that grow
 * from both ends (head grows up for commands, tail grows down for data).
 */

#include <string.h>
#include "pc/platform.h"

/* TwoHeadArena structure */
typedef struct {
    size_t size;
    char* buf_p;
    char* head_p;
    char* tail_p;
} TwoHeadArena;

typedef TwoHeadArena THA;

/* TwoHeadArena for Gfx */
typedef struct {
    size_t size;
    u64* buf_p;
    u64* head_p;
    u64* tail_p;
} TwoHeadArenaGfx;

typedef union {
    TwoHeadArena tha;
    TwoHeadArenaGfx thaGfx;
} THA_GA;

/* Forward declaration of Mtx and Vtx */
typedef union {
    s32 m[4][4];
    struct {
        u16 intPart[4][4];
        u16 fracPart[4][4];
    } fp;
    long long int force_structure_alignment;
} Mtx;

typedef struct {
    s16 ob[3];
    u16 flag;
    s16 tc[2];
    u8 cn[4];
} Vtx_t;

typedef union {
    Vtx_t v;
    long long int force_structure_alignment;
} Vtx;

typedef u64 Gfx;

/* Forward declarations */
static void* THA_allocAlign(TwoHeadArena* tha, size_t siz, int mask);
static int THA_getFreeBytesAlign(TwoHeadArena* tha, int mask);

/* ============================================================================
 * TwoHeadArena (THA) Implementation
 * ============================================================================ */

void THA_ct(TwoHeadArena* tha, char* p, size_t n) {
    tha->buf_p = p;
    tha->head_p = p;
    tha->tail_p = p + n;
    tha->size = n;
}

void THA_dt(TwoHeadArena* tha) {
    /* Nothing to do - memory is externally managed */
    (void)tha;
}

void* THA_getHeadPtr(TwoHeadArena* tha) {
    return tha->head_p;
}

void THA_setHeadPtr(TwoHeadArena* tha, void* p) {
    tha->head_p = (char*)p;
}

void* THA_getTailPtr(TwoHeadArena* tha) {
    return tha->tail_p;
}

void* THA_nextPtrN(TwoHeadArena* tha, size_t n) {
    void* result = tha->head_p;
    tha->head_p += n;
    return result;
}

void* THA_nextPtr1(TwoHeadArena* tha) {
    return THA_nextPtrN(tha, 1);
}

void* THA_alloc(TwoHeadArena* tha, size_t siz) {
    tha->tail_p -= siz;
    return tha->tail_p;
}

void* THA_alloc16(TwoHeadArena* tha, size_t siz) {
    return THA_allocAlign(tha, siz, 0xF);
}

static void* THA_allocAlign(TwoHeadArena* tha, size_t siz, int mask) {
    tha->tail_p = (char*)(((size_t)tha->tail_p - siz) & ~(size_t)mask);
    return tha->tail_p;
}

int THA_isCrash(TwoHeadArena* tha) {
    return tha->head_p > tha->tail_p;
}

void THA_init(TwoHeadArena* tha) {
    tha->head_p = tha->buf_p;
    tha->tail_p = tha->buf_p + tha->size;
}

int THA_getFreeBytes16(TwoHeadArena* tha) {
    return THA_getFreeBytesAlign(tha, 0xF);
}

int THA_getFreeBytes(TwoHeadArena* tha) {
    return (int)(tha->tail_p - tha->head_p);
}

static int THA_getFreeBytesAlign(TwoHeadArena* tha, int mask) {
    return (int)(((size_t)tha->tail_p & ~(size_t)mask) - (size_t)tha->head_p);
}

/* ============================================================================
 * THA_GA (Gfx Arena) Implementation
 * ============================================================================ */

void THA_GA_ct(THA_GA* tha_ga, Gfx* p, size_t n) {
    THA_ct(&tha_ga->tha, (char*)p, n);
}

void THA_GA_dt(THA_GA* tha_ga) {
    THA_dt(&tha_ga->tha);
}

int THA_GA_isCrash(THA_GA* tha_ga) {
    return THA_isCrash(&tha_ga->tha);
}

void THA_GA_init(THA_GA* tha_ga) {
    THA_init(&tha_ga->tha);
}

int THA_GA_getFreeBytes(THA_GA* tha_ga) {
    return THA_getFreeBytes(&tha_ga->tha);
}

void* THA_GA_getTailPtr(THA_GA* tha_ga) {
    return THA_getTailPtr(&tha_ga->tha);
}

void* THA_GA_nextPtrN(THA_GA* tha_ga, size_t n) {
    return THA_nextPtrN(&tha_ga->tha, n);
}

void* THA_GA_nextPtr1(THA_GA* tha_ga) {
    return THA_nextPtr1(&tha_ga->tha);
}

Gfx* THA_GA_NEXT_DISP(THA_GA* tha_ga) {
    Gfx* result = (Gfx*)tha_ga->thaGfx.head_p;
    tha_ga->thaGfx.head_p++;
    return result;
}

void* THA_GA_getHeadPtr(THA_GA* tha_ga) {
    return THA_getHeadPtr(&tha_ga->tha);
}

void THA_GA_setHeadPtr(THA_GA* tha_ga, void* p) {
    THA_setHeadPtr(&tha_ga->tha, p);
}

Mtx* THA_GA_alloc(THA_GA* tha_ga, size_t n) {
    return (Mtx*)THA_alloc(&tha_ga->tha, n);
}

Mtx* THA_GA_allocMtxN(THA_GA* tha_ga, size_t n) {
    return (Mtx*)THA_allocAlign(&tha_ga->tha, sizeof(Mtx) * n, 0x7);
}

Mtx* THA_GA_allocMtx1(THA_GA* tha_ga) {
    return THA_GA_allocMtxN(tha_ga, 1);
}

Vtx* THA_GA_allocVtxN(THA_GA* tha_ga, size_t n) {
    return (Vtx*)THA_allocAlign(&tha_ga->tha, sizeof(Vtx) * n, 0x7);
}

Vtx* THA_GA_allocVtx1(THA_GA* tha_ga) {
    return THA_GA_allocVtxN(tha_ga, 1);
}
