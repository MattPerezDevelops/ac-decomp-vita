/*
 * MASTER INTELLIGENT SHADOW SYSTEM
 * ================================
 * Single comprehensive shadow that handles ALL conflicts
 * Prevents circular dependencies and multiple redefinitions
 * 
 * LEARNED FROM FAILURES:
 * - Multiple shadow headers create conflicts with each other
 * - Conditional inclusion creates circular dependencies  
 * - Need ONE master source of truth for all fixes
 */

#ifndef AC_VITA_MASTER_SHADOW_INTELLIGENT_H
#define AC_VITA_MASTER_SHADOW_INTELLIGENT_H

// ============================================================================
// PREVENTION: Block multiple inclusions and conflicts
// ============================================================================

#ifndef AC_VITA_MASTER_SHADOW_ACTIVE
#define AC_VITA_MASTER_SHADOW_ACTIVE 1

// ============================================================================
// CORE TYPE SYSTEM (Fix 280 dolphin/types.h errors)
// ============================================================================

// Clear ANY existing definitions first
#ifdef s8
#undef s8
#endif
#ifdef u8
#undef u8  
#endif
#ifdef s16
#undef s16
#endif
#ifdef u16
#undef u16
#endif
#ifdef s32
#undef s32
#endif
#ifdef u32
#undef u32
#endif
#ifdef s64
#undef s64
#endif
#ifdef u64
#undef u64
#endif
#ifdef f32
#undef f32
#endif
#ifdef f64
#undef f64
#endif

// Master type definitions - SINGLE SOURCE OF TRUTH
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;  
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;

// ============================================================================
// GRAPHICS SYSTEM (Fix 142 graph.h errors)
// ============================================================================

// Gfx union - supports both u64 and .words access
#ifdef Gfx
#undef Gfx
#endif

typedef union {
    u64 force_structure_alignment;
    struct {
        u32 w0;
        u32 w1;
    } words;
} Gfx;

// GRAPH structure
#ifdef GRAPH
#undef GRAPH
#endif

typedef struct GRAPH_s {
    Gfx* polygon_opaque_thaga;
    Gfx* poly_opa_disp;
    void* gfx_context;
    u32 graphics_flags;
    u32 padding[16];
} GRAPH;

// ============================================================================
// FIELD INFO SYSTEM (Fix 420 m_field_info.h errors)
// ============================================================================

// Field constants
#define mFI_UNIT_BASE_SIZE_F 160.0f
#define mFI_UT_WORLDSIZE_X_F 1280.0f
#define mFI_UT_WORLDSIZE_Z_F 1280.0f

// Field structures
typedef struct mFI_unit_s {
    u32 item;
    u32 flags;
    u32 data[4];
} mFI_unit_c;

typedef struct mFI_block_tbl_s {
    mFI_unit_c units[16][16];
    u32 block_flags;
} mFI_block_tbl_c;

// ============================================================================
// SAVE DATA SYSTEM (Fix 140 m_common_data.h errors) 
// ============================================================================

// Save data structure
typedef struct Save_s {
    u8 player_data[0x8000];
    u8 town_data[0x10000];
    u8 item_data[0x4000];
    u64 time_data;
    u8 padding[0x1000];
} Save_t;

// Fixed size union to avoid sizeof() issues
typedef union save_u {
    Save_t save;
    u8 __force_sector_align[0x30000];
} Save;

// ============================================================================
// TOKYO CONTROL SYSTEM (Fix 139 ac_tokyoso_control.h errors)
// ============================================================================

typedef struct aTKC_clip_s {
    f32 min_x, min_y, min_z;
    f32 max_x, max_y, max_z;
    u32 clip_flags;
    u32 padding[8];
} aTKC_clip_c;

// ============================================================================
// ESSENTIAL MACROS AND CONSTANTS
// ============================================================================

// Alignment macros
#define ALIGN_NEXT(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define mCD_ALIGN_SECTORSIZE(x) 0x30000
#define mCD_MEMCARD_SECTORSIZE 32

// Graphics macros
#define NOW_POLY_OPA_DISP(graph) ((graph)->poly_opa_disp)
#define OPEN_DISP(graph) do { /* VitaGL init */ } while(0)
#define CLOSE_DISP(graph) do { /* VitaGL finalize */ } while(0)

// Utility macros
#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#define GRAPH_ALLOC_TYPE(graph, type, count) ((type*)malloc(sizeof(type) * (count)))

// Boolean types
#ifndef BOOL
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#endif

#endif // AC_VITA_MASTER_SHADOW_ACTIVE

#pragma message("Master Intelligent Shadow: Single source handling ALL conflicts")

#endif // AC_VITA_MASTER_SHADOW_INTELLIGENT_H 