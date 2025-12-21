#include "m_player_call.h"

#ifdef TARGET_PC
#include <stdio.h>
#endif

#include "m_player.h"
#include "m_name_table.h"
#include "m_play.h"

static mActor_proc Player_actor_ct_func;
static mActor_proc Player_actor_dt_func;
static mActor_proc Player_actor_move_func;
static mActor_proc Player_actor_draw_func;

void Player_actor_ct_call(ACTOR* actor, GAME* game);
void Player_actor_dt_call(ACTOR* actor, GAME* game);
void Player_actor_move_call(ACTOR* actor, GAME* game);
void Player_actor_draw_call(ACTOR* actor, GAME* game);

ACTOR_PROFILE Player_Profile = {
    mAc_PROFILE_PLAYER,
    ACTOR_PART_PLAYER,
    ACTOR_STATE_CAN_MOVE_IN_DEMO_SCENES | ACTOR_STATE_26 | ACTOR_STATE_25 | ACTOR_STATE_NO_MOVE_WHILE_CULLED |
        ACTOR_STATE_NO_DRAW_WHILE_CULLED | ACTOR_STATE_2 | ACTOR_STATE_0,
    EMPTY_NO,
    ACTOR_OBJ_BANK_KEEP,
    sizeof(PLAYER_ACTOR),
    Player_actor_ct_call,
    Player_actor_dt_call,
    Player_actor_move_call,
    Player_actor_draw_call,
    NULL,
};

static void initfunc(GAME_PLAY* play) {
    Submenu* submenu = &play->submenu;

    Player_actor_ct_func = mSM_ovlptr_dllcnv(&Player_actor_ct, submenu, 1);
    Player_actor_dt_func = mSM_ovlptr_dllcnv(&Player_actor_dt, submenu, 1);
    Player_actor_move_func = mSM_ovlptr_dllcnv(&Player_actor_move, submenu, 1);
    Player_actor_draw_func = mSM_ovlptr_dllcnv(&Player_actor_draw, submenu, 1);
}

void Player_actor_ct_call(ACTOR* actor, GAME* game) {
#ifdef TARGET_PC
    /* PC Port: Skip DLL loading, just call stubbed function directly */
    (void)actor; (void)game;
    printf("[PLAYER] Player_actor_ct_call - PC stub (skip DLL)\n");
    fflush(stdout);
#else
    GAME_PLAY* play = (GAME_PLAY*)game;

    load_player(&play->submenu);
    initfunc(play);
    Player_actor_ct_func(actor, game);
#endif
}

void Player_actor_dt_call(ACTOR* actor, GAME* game) {
#ifdef TARGET_PC
    (void)actor; (void)game;
    /* PC stub - do nothing */
#else
    GAME_PLAY* play = (GAME_PLAY*)game;
    load_player(&play->submenu);
    Player_actor_dt_func(actor, game);
#endif
}

void Player_actor_move_call(ACTOR* actor, GAME* game) {
#ifdef TARGET_PC
    /* PC Port: Simple keyboard movement */
    extern void input_get_movement(float* dx, float* dz);
    float dx, dz;
    static int move_log = 0;

    (void)game;

    input_get_movement(&dx, &dz);

    if (dx != 0.0f || dz != 0.0f) {
        /* Move player by input * speed (50 units/frame) */
        actor->world.position.x += dx * 50.0f;
        actor->world.position.z += dz * 50.0f;

        if (move_log < 20) {
            printf("[PLAYER_MOVE] input(%.1f, %.1f) -> pos(%.1f, %.1f, %.1f)\n",
                   dx, dz,
                   actor->world.position.x,
                   actor->world.position.y,
                   actor->world.position.z);
            fflush(stdout);
            move_log++;
        }
    }
#else
    GAME_PLAY* play = (GAME_PLAY*)game;
    load_player(&play->submenu);
    Player_actor_move_func(actor, game);
#endif
}

void Player_actor_draw_call(ACTOR* actor, GAME* game) {
#ifdef TARGET_PC
    /* PC Port: Player cube is now drawn in ac_field_draw.c
     * using the correct graph context (BG_OPA buffer). */
    (void)actor;
    (void)game;
#else
    GAME_PLAY* play = (GAME_PLAY*)game;
    load_player(&play->submenu);
    Player_actor_draw_func(actor, game);
#endif
}
