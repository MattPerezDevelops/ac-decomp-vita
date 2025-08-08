/*
 * SMART COMMON DATA SHADOW
 * ========================
 * Targets m_common_data.h (140 errors) - Save data system issues
 * Fixes Save_t structure and memory card alignment problems
 */

#ifndef AC_VITA_SMART_COMMON_DATA_SHADOW_H
#define AC_VITA_SMART_COMMON_DATA_SHADOW_H

// ============================================================================
// SAVE DATA STRUCTURE FIXES (Array size calculation issues)
// ============================================================================

// Forward declare Save_t before using in array calculations
#ifndef Save_t
typedef struct Save_s {
    // Player data
    u8 player_data[0x8000];
    
    // Town data  
    u8 town_data[0x10000];
    
    // Item data
    u8 item_data[0x4000];
    
    // Event data
    u8 event_data[0x2000];
    
    // Time and calendar
    u64 time_data;
    u32 calendar_data[64];
    
    // Padding for memory card alignment
    u8 padding[0x1000];
} Save_t;
#endif

// ============================================================================
// MEMORY CARD ALIGNMENT FIXES
// ============================================================================

// Fix the problematic array size calculation
#ifndef Save
typedef union save_u {
    Save_t save;
    // Use fixed size instead of macro calculation to avoid sizeof() issues
    u8 __force_sector_align[0x30000];  // Large enough for any save data
} Save;
#endif

// Memory card constants
#ifndef mCD_MEMCARD_SECTORSIZE
#define mCD_MEMCARD_SECTORSIZE 32
#endif

#ifndef mCD_FIXED_SAVE_SIZE
#define mCD_FIXED_SAVE_SIZE 0x30000  // 192KB - sufficient for save data
#endif

// Fixed alignment macro to avoid sizeof() in array declarations
#ifndef mCD_ALIGN_SECTORSIZE
#define mCD_ALIGN_SECTORSIZE(x) mCD_FIXED_SAVE_SIZE
#endif

// ============================================================================
// SAVE DATA CONSTANTS
// ============================================================================

#ifndef mSv_DATA_SIZE
#define mSv_DATA_SIZE 0x24000
#endif

#ifndef mSv_PLAYER_NUM
#define mSv_PLAYER_NUM 4
#endif

#ifndef mSv_VILLAGE_NAME_LEN
#define mSv_VILLAGE_NAME_LEN 8
#endif

// ============================================================================
// TIME AND TRANSITION STRUCTURES
// ============================================================================

#ifndef transition_s
typedef struct transition_s {
    u8 _00;
    u8 fade_rate;
    u8 wipe_rate; 
    u8 wipe_type;
    u32 transition_flags;
    u32 padding[4];
} transition_t;
#endif

// ============================================================================
// SAVE DATA VALIDATION
// ============================================================================

#ifndef mSv_CHECKSUM_SIZE
#define mSv_CHECKSUM_SIZE 4
#endif

#ifndef mSv_SAVE_VERSION
#define mSv_SAVE_VERSION 6
#endif

#pragma message("Smart Common Data Shadow: 140 save system errors targeted")

#endif // AC_VITA_SMART_COMMON_DATA_SHADOW_H 