/**
 * @file scene_loader.c
 * @brief PC-specific scene data loader with 64-bit pointer support
 *
 * On N64/GC, scene data uses static arrays with (u32)ptr casts.
 * On 64-bit PC, these casts truncate pointers and fail to compile.
 *
 * This loader builds scene command tables at runtime with proper 64-bit pointers.
 * The actual data arrays (actor data, control actors, etc.) are defined normally
 * and work on any platform.
 */

#include "m_scene.h"
#include "m_actor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Scene Data Definitions (shared with original files)
 * ============================================================================ */

/* SCENE_FG (Village) - test_fd_npc_land equivalent */
static Actor_data scene_fg_player_data[] = {
    {
        mAc_PROFILE_PLAYER, /* profile */
        { 2240, 0, 1600 },  /* position */
        { 0, 0, 0 },        /* rotation */
        0                   /* ct arg */
    },
};

static s16 scene_fg_ctrl_actor_data[] = {
    mAc_PROFILE_EFFECTBG,        /* 0 */
    mAc_PROFILE_BIRTH_CONTROL,   /* 1 */
    /* mAc_PROFILE_NPC, */       /* 2 - Not approved for PC yet */
    mAc_PROFILE_STRUCTURE,       /* 3 */
    /* mAc_PROFILE_INSECT, */    /* 4 - Not approved */
    /* mAc_PROFILE_TOOLS, */     /* 5 - Not approved */
    /* mAc_PROFILE_HANDOVERITEM, */  /* 6 - Not approved */
    /* mAc_PROFILE_EFFECT_CONTROL, */ /* 7 - Not approved */
    /* mAc_PROFILE_SHOP_LEVEL, */ /* 8 - Not approved */
    /* mAc_PROFILE_QUEST_MANAGER, */ /* 9 - Not approved */
    /* mAc_PROFILE_EVENT_MANAGER, */ /* 10 - Not approved */
    /* mAc_PROFILE_WEATHER, */   /* 11 - Need to add */
    /* mAc_PROFILE_SET_MANAGER, */ /* 12 - Not approved */
    /* mAc_PROFILE_GYOEI, */     /* 13 - Not approved */
    /* mAc_PROFILE_SET_NPC_MANAGER, */ /* 14 - Not approved */
    /* mAc_PROFILE_BALL, */      /* 15 - Not approved */
    /* mAc_PROFILE_MSCORE_CONTROL, */ /* 16 - Not approved */
};
#define SCENE_FG_CTRL_ACTOR_COUNT (sizeof(scene_fg_ctrl_actor_data) / sizeof(scene_fg_ctrl_actor_data[0]))

static Actor_data scene_fg_actor_data[] = {
    {
        mAc_PROFILE_DUMMY, /* profile - placeholder actor */
        { 340, 0, 430 },   /* position */
        { 0, 0, 0 },       /* rotation */
        -1                 /* ct arg */
    },
};
#define SCENE_FG_ACTOR_COUNT (sizeof(scene_fg_actor_data) / sizeof(scene_fg_actor_data[0]))

static s16 scene_fg_object_bank[] = {
    ACTOR_OBJ_BANK_KEEP, /* Minimal bank */
};
#define SCENE_FG_OBJECT_BANK_COUNT (sizeof(scene_fg_object_bank) / sizeof(scene_fg_object_bank[0]))

/* ============================================================================
 * Runtime Scene Command Builder
 * ============================================================================ */

/**
 * Helper to set a Scene_Word_u entry as a control actor pointer.
 * Uses union member directly - no u32 cast needed.
 */
static void scene_set_ctrl_actors(Scene_Word_u* cmd, int num, s16* data) {
    cmd->control_actor.type = mSc_SCENE_DATA_TYPE_CTRL_ACTOR_PTR;
    cmd->control_actor.num_ctrl_actors = num;
    cmd->control_actor.ctrl_actor_profile_p = data;
}

/**
 * Helper to set a Scene_Word_u entry as a player/actor pointer.
 */
static void scene_set_actors(Scene_Word_u* cmd, u8 type, int num, Actor_data* data) {
    cmd->actor.type = type;
    cmd->actor.num_actors = num;
    cmd->actor.data_p = data;
}

/**
 * Helper to set a Scene_Word_u entry as an object bank pointer.
 */
static void scene_set_object_bank(Scene_Word_u* cmd, int num, s16* data) {
    cmd->object_bank.type = mSc_SCENE_DATA_TYPE_OBJECT_EXCHANGE_BANK_PTR;
    cmd->object_bank.num_banks = num;
    cmd->object_bank.banks_p = data;
}

/**
 * Helper to set field construct parameters.
 */
static void scene_set_field_ct(Scene_Word_u* cmd, u8 item_type, u8 bg_num,
                                u16 bg_disp_size, u8 room_type, u8 draw_type) {
    cmd->field_ct.type = mSc_SCENE_DATA_TYPE_FIELD_CT;
    cmd->field_ct.item_type = item_type;
    cmd->field_ct.bg_num = bg_num;
    /* Set field_ct struct fields directly - don't use misc.param3 packing
     * which has different byte layout on little-endian machines */
    cmd->field_ct.bg_disp_size = bg_disp_size;
    cmd->field_ct.room_type = room_type;
    cmd->field_ct.draw_type = draw_type;
}

/**
 * Helper to set sound parameters.
 */
static void scene_set_sound(Scene_Word_u* cmd, u8 param0, u8 param1) {
    cmd->misc.type = mSc_SCENE_DATA_TYPE_SOUND;
    cmd->misc.param0 = param0;
    cmd->misc.param1 = param1;
    cmd->misc.param2 = 0;
    cmd->misc.param3 = 0;
}

/**
 * Helper to set end marker.
 */
static void scene_set_end(Scene_Word_u* cmd) {
    cmd->misc.type = mSc_SCENE_DATA_TYPE_END;
    cmd->misc.param0 = 0;
    cmd->misc.param1 = 0;
    cmd->misc.param2 = 0;
    cmd->misc.param3 = 0;
}

/* ============================================================================
 * SCENE_TITLE_DEMO Data (title screen)
 * ============================================================================ */

static Actor_data scene_title_demo_player_data[] = {
    {
        mAc_PROFILE_PLAYER, /* profile */
        { 2240, 0, 1600 },  /* position */
        { 0, 0, 0 },        /* rotation */
        0                   /* ct arg */
    },
};

static s16 scene_title_demo_ctrl_actor_data[] = {
    mAc_PROFILE_EFFECTBG,       /* 0 - Background effects */
    mAc_PROFILE_BIRTH_CONTROL,  /* 1 - Item spawner (guarded for PC) */
    /* mAc_PROFILE_NPC, */      /* 2 - Not approved */
    mAc_PROFILE_STRUCTURE,      /* 3 - Structure renderer */
    /* mAc_PROFILE_TOOLS, */    /* 4 - Not approved */
    /* mAc_PROFILE_HANDOVERITEM, */ /* 5 - Not approved */
    /* mAc_PROFILE_EFFECT_CONTROL, */ /* 6 - Not approved */
    /* mAc_PROFILE_WEATHER, */  /* 7 - Not approved */
    mAc_PROFILE_ANIMAL_LOGO,    /* 8 - Title screen logo - CRITICAL */
    /* mAc_PROFILE_QUEST_MANAGER, */ /* 9 - Not approved */
};
#define SCENE_TITLE_DEMO_CTRL_ACTOR_COUNT (sizeof(scene_title_demo_ctrl_actor_data) / sizeof(scene_title_demo_ctrl_actor_data[0]))

static Actor_data scene_title_demo_actor_data[] = {
    {
        mAc_PROFILE_DUMMY, /* profile - placeholder */
        { 340, 0, 430 },   /* position */
        { 0, 0, 0 },       /* rotation */
        -1                 /* ct arg */
    },
};
#define SCENE_TITLE_DEMO_ACTOR_COUNT (sizeof(scene_title_demo_actor_data) / sizeof(scene_title_demo_actor_data[0]))

static s16 scene_title_demo_object_bank[] = {
    ACTOR_OBJ_BANK_KEEP, /* Minimal bank */
};
#define SCENE_TITLE_DEMO_OBJECT_BANK_COUNT (sizeof(scene_title_demo_object_bank) / sizeof(scene_title_demo_object_bank[0]))

/* ============================================================================
 * Scene Command Tables (built at runtime)
 * ============================================================================ */

static Scene_Word_u* g_scene_title_demo_info = NULL;
static Scene_Word_u* g_scene_fg_info = NULL;

/**
 * Build SCENE_TITLE_DEMO (Title Screen) scene command table.
 */
static Scene_Word_u* build_scene_title_demo(void) {
    /* Allocate command table (7 commands + 1 end) */
    Scene_Word_u* info = (Scene_Word_u*)malloc(8 * sizeof(Scene_Word_u));
    if (!info) {
        printf("[SCENE_LOADER] Failed to allocate SCENE_TITLE_DEMO command table!\n");
        return NULL;
    }

    int idx = 0;

    /* Sound data */
    scene_set_sound(&info[idx++], 0, 0);

    /* Player data */
    scene_set_actors(&info[idx++], mSc_SCENE_DATA_TYPE_PLAYER_PTR, 1, scene_title_demo_player_data);

    /* Control actors */
    scene_set_ctrl_actors(&info[idx++], SCENE_TITLE_DEMO_CTRL_ACTOR_COUNT, scene_title_demo_ctrl_actor_data);

    /* Actor data */
    scene_set_actors(&info[idx++], mSc_SCENE_DATA_TYPE_ACTOR_PTR, SCENE_TITLE_DEMO_ACTOR_COUNT, scene_title_demo_actor_data);

    /* Object banks */
    scene_set_object_bank(&info[idx++], SCENE_TITLE_DEMO_OBJECT_BANK_COUNT, scene_title_demo_object_bank);

    /* Field construct parameters - same as SCENE_FG but with bg_disp_size=0x2000 */
    scene_set_field_ct(&info[idx++], mSc_ITEM_TYPE_BGITEM, 4, 0x2000,
                       mSc_ROOM_TYPE_OUTDOORS, FIELD_DRAW_TYPE_OUTDOORS);

    /* End marker */
    scene_set_end(&info[idx++]);

    printf("[SCENE_LOADER] Built SCENE_TITLE_DEMO command table (%d commands)\n", idx);
    printf("[SCENE_LOADER]   ctrl_actors=%d player_data=%p ctrl_data=%p\n",
           (int)SCENE_TITLE_DEMO_CTRL_ACTOR_COUNT, (void*)scene_title_demo_player_data,
           (void*)scene_title_demo_ctrl_actor_data);

    return info;
}

/**
 * Build SCENE_FG (Village) scene command table.
 */
static Scene_Word_u* build_scene_fg(void) {
    /* Allocate command table (7 commands + 1 end) */
    Scene_Word_u* info = (Scene_Word_u*)malloc(8 * sizeof(Scene_Word_u));
    if (!info) {
        printf("[SCENE_LOADER] Failed to allocate SCENE_FG command table!\n");
        return NULL;
    }

    int idx = 0;

    /* Sound data */
    scene_set_sound(&info[idx++], 0, 0);

    /* Player data */
    scene_set_actors(&info[idx++], mSc_SCENE_DATA_TYPE_PLAYER_PTR, 1, scene_fg_player_data);

    /* Control actors */
    scene_set_ctrl_actors(&info[idx++], SCENE_FG_CTRL_ACTOR_COUNT, scene_fg_ctrl_actor_data);

    /* Actor data */
    scene_set_actors(&info[idx++], mSc_SCENE_DATA_TYPE_ACTOR_PTR, SCENE_FG_ACTOR_COUNT, scene_fg_actor_data);

    /* Object banks */
    scene_set_object_bank(&info[idx++], SCENE_FG_OBJECT_BANK_COUNT, scene_fg_object_bank);

    /* Field construct parameters */
    /* mSc_ITEM_TYPE_BGITEM=0, bg_num=4, bg_disp_size=0x1C00, room_type=OUTDOORS=0, draw_type=OUTDOORS=0 */
    scene_set_field_ct(&info[idx++], mSc_ITEM_TYPE_BGITEM, 4, 0x1C00,
                       mSc_ROOM_TYPE_OUTDOORS, FIELD_DRAW_TYPE_OUTDOORS);

    /* End marker */
    scene_set_end(&info[idx++]);

    printf("[SCENE_LOADER] Built SCENE_FG command table (%d commands)\n", idx);
    printf("[SCENE_LOADER]   ctrl_actors=%d player_data=%p ctrl_data=%p\n",
           (int)SCENE_FG_CTRL_ACTOR_COUNT, (void*)scene_fg_player_data,
           (void*)scene_fg_ctrl_actor_data);

    return info;
}

/* ============================================================================
 * Public API
 * ============================================================================ */

/**
 * Initialize the scene loader.
 * Builds runtime scene command tables for all scenes.
 */
void scene_loader_init(void) {
    printf("[SCENE_LOADER] Initializing scene loader...\n");

    /* Build SCENE_TITLE_DEMO (Title Screen) */
    g_scene_title_demo_info = build_scene_title_demo();

    /* Build SCENE_FG (Village) */
    g_scene_fg_info = build_scene_fg();

    printf("[SCENE_LOADER] Scene loader initialized\n");
}

/**
 * Get scene data for a given scene ID.
 * Returns the runtime-built Scene_Word_u array.
 */
Scene_Word_u* scene_loader_get(int scene_id) {
    switch (scene_id) {
        case SCENE_TITLE_DEMO:
            if (!g_scene_title_demo_info) {
                g_scene_title_demo_info = build_scene_title_demo();
            }
            return g_scene_title_demo_info;

        case SCENE_FG:
            if (!g_scene_fg_info) {
                g_scene_fg_info = build_scene_fg();
            }
            return g_scene_fg_info;

        default:
            printf("[SCENE_LOADER] Scene %d not yet implemented for PC\n", scene_id);
            return NULL;
    }
}

/**
 * Cleanup scene loader resources.
 */
void scene_loader_cleanup(void) {
    if (g_scene_fg_info) {
        free(g_scene_fg_info);
        g_scene_fg_info = NULL;
    }
}

/**
 * Debug: Print scene command table contents.
 */
void scene_loader_debug_print(Scene_Word_u* info) {
    if (!info) {
        printf("[SCENE_LOADER] NULL scene data\n");
        return;
    }

    int idx = 0;
    while (info[idx].misc.type != mSc_SCENE_DATA_TYPE_END) {
        Scene_Word_u* cmd = &info[idx];
        printf("[SCENE_LOADER] cmd[%d] type=%d", idx, cmd->misc.type);

        switch (cmd->misc.type) {
            case mSc_SCENE_DATA_TYPE_PLAYER_PTR:
                printf(" PLAYER data=%p", (void*)cmd->actor.data_p);
                break;
            case mSc_SCENE_DATA_TYPE_CTRL_ACTOR_PTR:
                printf(" CTRL_ACTORS num=%d data=%p",
                       cmd->control_actor.num_ctrl_actors,
                       (void*)cmd->control_actor.ctrl_actor_profile_p);
                break;
            case mSc_SCENE_DATA_TYPE_ACTOR_PTR:
                printf(" ACTORS num=%d data=%p",
                       cmd->actor.num_actors, (void*)cmd->actor.data_p);
                break;
            case mSc_SCENE_DATA_TYPE_OBJECT_EXCHANGE_BANK_PTR:
                printf(" OBJ_BANK num=%d data=%p",
                       cmd->object_bank.num_banks, (void*)cmd->object_bank.banks_p);
                break;
            case mSc_SCENE_DATA_TYPE_FIELD_CT:
                printf(" FIELD_CT item_type=%d bg_num=%d",
                       cmd->field_ct.item_type, cmd->field_ct.bg_num);
                break;
            case mSc_SCENE_DATA_TYPE_SOUND:
                printf(" SOUND");
                break;
        }
        printf("\n");
        idx++;
    }
    printf("[SCENE_LOADER] cmd[%d] type=%d END\n", idx, mSc_SCENE_DATA_TYPE_END);
}
