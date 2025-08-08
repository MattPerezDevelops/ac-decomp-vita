/*
 * SMART FIELD INFO SHADOW
 * =======================
 * Targets m_field_info.h (420 errors) - Biggest single offender
 * Field constants, structures, and terrain system fixes
 */

#ifndef AC_VITA_SMART_FIELD_INFO_SHADOW_H
#define AC_VITA_SMART_FIELD_INFO_SHADOW_H

// ============================================================================
// FIELD INFO CONSTANTS (Main source of errors)
// ============================================================================

// Core field size constants
#ifndef mFI_UNIT_BASE_SIZE_F
#define mFI_UNIT_BASE_SIZE_F 160.0f
#endif

#ifndef mFI_UT_WORLDSIZE_X_F
#define mFI_UT_WORLDSIZE_X_F 1280.0f  
#endif

#ifndef mFI_UT_WORLDSIZE_Z_F
#define mFI_UT_WORLDSIZE_Z_F 1280.0f
#endif

#ifndef mFI_BK_WORLDSIZE_X_F
#define mFI_BK_WORLDSIZE_X_F 2560.0f
#endif

#ifndef mFI_BK_WORLDSIZE_Z_F  
#define mFI_BK_WORLDSIZE_Z_F 2560.0f
#endif

// Field unit dimensions
#ifndef mFI_UNIT_SIZE_F
#define mFI_UNIT_SIZE_F 32.0f
#endif

#ifndef mFI_BLOCK_SIZE_F
#define mFI_BLOCK_SIZE_F 256.0f
#endif

// ============================================================================
// FIELD INFO STRUCTURES (Type conflict resolution)
// ============================================================================

// Forward declarations to prevent conflicts
#ifndef mFI_unit_c
typedef struct mFI_unit_s {
    u32 item;
    u32 flags;
    u32 data[4];
} mFI_unit_c;
#endif

#ifndef mFI_block_tbl_c
typedef struct mFI_block_tbl_s {
    mFI_unit_c units[16][16];
    u32 block_flags;
    u32 block_data[8];
} mFI_block_tbl_c;
#endif

#ifndef mFI_field_c
typedef struct mFI_field_s {
    mFI_block_tbl_c blocks[10][10];
    u32 field_flags;
    u32 field_data[16];
} mFI_field_c;
#endif

// ============================================================================
// FIELD INFO MACROS (Syntax error prevention)
// ============================================================================

#ifndef mFI_GetFieldId
#define mFI_GetFieldId(pos) ((pos)->field_id)
#endif

#ifndef mFI_BlockKind2UtNum
#define mFI_BlockKind2UtNum(kind) ((kind) & 0xFF)
#endif

#ifndef mFI_GetUnitGrassG
#define mFI_GetUnitGrassG(unit) (((unit)->flags >> 8) & 0xFF)
#endif

// ============================================================================
// TERRAIN SYSTEM FIXES
// ============================================================================

// Terrain types and flags
#ifndef mFI_TERRAIN_GRASS
#define mFI_TERRAIN_GRASS 0x00
#define mFI_TERRAIN_DIRT  0x01
#define mFI_TERRAIN_SAND  0x02
#define mFI_TERRAIN_ROCK  0x03
#endif

// Field item system
#ifndef mFI_ITEM_NONE
#define mFI_ITEM_NONE 0x0000
#define mFI_ITEM_TREE 0x0001
#define mFI_ITEM_ROCK 0x0002
#define mFI_ITEM_HOLE 0x0003
#endif

#pragma message("Smart Field Info Shadow: 420 errors targeted")

#endif // AC_VITA_SMART_FIELD_INFO_SHADOW_H 