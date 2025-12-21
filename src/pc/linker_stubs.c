/**
 * @file linker_stubs.c
 * @brief Minimal stubs for undefined symbols without header conflicts
 *
 * This file provides stub implementations for functions/data that are needed
 * to link but don't have implementations yet. It intentionally doesn't include
 * the original headers to avoid type conflicts.
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Include graph.h for GRAPH structure and GRAPH_ALLOC */
#include "graph.h"
#include "PR/mbi.h"
#include "types.h"

/* Global player pointer - set when player actor spawns */

/* GBI structures for stubs (that don't conflict with actual types) */
typedef struct { u32 words[2]; } Gfx_stub;
typedef struct { f32 x, y, z; } xyz_stub;

/* ============================================================================
 * Math functions
 * ============================================================================ */
f32 sin_s(s16 angle) { return sinf(angle * 3.14159265f / 32768.0f); }
f32 cos_s(s16 angle) { return cosf(angle * 3.14159265f / 32768.0f); }
void add_calc(f32* pValue, f32 target, f32 fraction, f32 maxStep, f32 minStep) {
    (void)pValue; (void)target; (void)fraction; (void)maxStep; (void)minStep;
}
s16 add_calc_short_angle2(s16* pValue, s16 target, f32 fraction, s16 maxStep, s16 minStep) {
    (void)pValue; (void)target; (void)fraction; (void)maxStep; (void)minStep;
    return 0;
}
f32 chase_f(f32* pValue, f32 target, f32 step) { (void)pValue; (void)target; (void)step; return 0.0f; }
f32 inter_float(f32 a, f32 b, f32 t) { return a + (b - a) * t; }
f32 get_percent(int max, int min, int x) { (void)max; (void)min; (void)x; return 0.0f; }
f32 get_percent_forAccelBrake(f32 now, f32 start, f32 end, f32 accel, f32 brake) {
    (void)now; (void)start; (void)end; (void)accel; (void)brake; return 0.0f;
}
void xyz_t_move(void* dst, const void* src) { if(dst && src) memcpy(dst, src, 12); }
void xyz_t_move_s_xyz(void* dst, const void* src) { (void)dst; (void)src; }
void xyz_t_add(void* dst, const void* a, const void* b) { (void)dst; (void)a; (void)b; }
void xyz_t_sub(void* dst, const void* a, const void* b) { (void)dst; (void)a; (void)b; }
void ZeroVec(void* v) {
    xyz_stub* _v = (xyz_stub*)v;
    if(_v) { _v->x = _v->y = _v->z = 0.0f; }
}
void ZeroSVec(void* v) {
    s16* _v = (s16*)v;
    if(_v) { _v[0] = _v[1] = _v[2] = 0; }
}
s16 search_position_angleY(const void* a, const void* b) { (void)a; (void)b; return 0; }
f32 search_position_distance(const void* a, const void* b) { (void)a; (void)b; return 0.0f; }
f32 search_position_distanceXZ(const void* a, const void* b) { (void)a; (void)b; return 0.0f; }

/* Lookup table */
f32 atans_table[0x801] = {0};

/* ============================================================================
 * Matrix Stack System
 * ============================================================================ */

/* MtxF is a 4x4 float matrix */
typedef struct {
    f32 mf[4][4];
} MtxF_stub;

#define MATRIX_STACK_SIZE 20
static MtxF_stub g_matrix_stack[MATRIX_STACK_SIZE];
static int g_matrix_stack_ptr = 0;

/* Initialize matrix to identity */
static void mtxf_identity(MtxF_stub* m) {
    memset(m->mf, 0, sizeof(m->mf));
    m->mf[0][0] = 1.0f;
    m->mf[1][1] = 1.0f;
    m->mf[2][2] = 1.0f;
    m->mf[3][3] = 1.0f;
}

/* Initialize matrix stack - called by game during init */
void new_Matrix(void* game) {
    (void)game;
    g_matrix_stack_ptr = 0;
    mtxf_identity(&g_matrix_stack[0]);
    printf("[new_Matrix] stack=%p lower32=0x%08X\n",
           (void*)g_matrix_stack, (u32)(uintptr_t)g_matrix_stack);
    fflush(stdout);
}

/* Get current matrix */
MtxF_stub* get_Matrix_now(void) {
    return &g_matrix_stack[g_matrix_stack_ptr];
}

void Matrix_put(const void* mf) {
    if (mf) memcpy(&g_matrix_stack[g_matrix_stack_ptr], mf, sizeof(MtxF_stub));
}

void Matrix_push(void) {
    if (g_matrix_stack_ptr < MATRIX_STACK_SIZE - 1) {
        memcpy(&g_matrix_stack[g_matrix_stack_ptr + 1],
               &g_matrix_stack[g_matrix_stack_ptr], sizeof(MtxF_stub));
        g_matrix_stack_ptr++;
    }
}

void Matrix_pull(void) {
    if (g_matrix_stack_ptr > 0) g_matrix_stack_ptr--;
}

void Matrix_copy_MtxF(void* dst, const void* src) {
    if (dst && src) memcpy(dst, src, sizeof(MtxF_stub));
}

void Matrix_mult(const void* mf, int mode) {
    if (!mf) return;
    const MtxF_stub* new_mtx = (const MtxF_stub*)mf;
    MtxF_stub* m = get_Matrix_now();

    if (mode == 0) { /* LOAD - replace current matrix */
        memcpy(m->mf, new_mtx->mf, sizeof(m->mf));
    } else { /* MULT - multiply: result = current * new */
        MtxF_stub temp;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                temp.mf[i][j] = m->mf[i][0] * new_mtx->mf[0][j] +
                                m->mf[i][1] * new_mtx->mf[1][j] +
                                m->mf[i][2] * new_mtx->mf[2][j] +
                                m->mf[i][3] * new_mtx->mf[3][j];
            }
        }
        memcpy(m->mf, temp.mf, sizeof(m->mf));
    }
}

void Matrix_translate(f32 x, f32 y, f32 z, int mode) {
    MtxF_stub* m = get_Matrix_now();
    if (mode == 0) { /* LOAD */
        mtxf_identity(m);
    }
    m->mf[3][0] += x;
    m->mf[3][1] += y;
    m->mf[3][2] += z;
}

void Matrix_scale(f32 x, f32 y, f32 z, int mode) {
    MtxF_stub* m = get_Matrix_now();
    if (mode == 0) { /* LOAD */
        mtxf_identity(m);
        m->mf[0][0] = x;
        m->mf[1][1] = y;
        m->mf[2][2] = z;
    } else { /* MULT */
        m->mf[0][0] *= x; m->mf[0][1] *= x; m->mf[0][2] *= x; m->mf[0][3] *= x;
        m->mf[1][0] *= y; m->mf[1][1] *= y; m->mf[1][2] *= y; m->mf[1][3] *= y;
        m->mf[2][0] *= z; m->mf[2][1] *= z; m->mf[2][2] *= z; m->mf[2][3] *= z;
    }
}

void Matrix_RotateX(f32 angle, int mode) {
    MtxF_stub* m = get_Matrix_now();
    f32 c = cosf(angle);
    f32 s = sinf(angle);

    if (mode == 0) { /* LOAD */
        mtxf_identity(m);
        m->mf[1][1] = c;  m->mf[1][2] = s;
        m->mf[2][1] = -s; m->mf[2][2] = c;
    } else { /* MULT */
        /* Multiply current matrix by rotation matrix */
        for (int i = 0; i < 4; i++) {
            f32 y = m->mf[i][1];
            f32 z = m->mf[i][2];
            m->mf[i][1] = y * c - z * s;
            m->mf[i][2] = y * s + z * c;
        }
    }
}

void Matrix_RotateY(f32 angle, int mode) {
    MtxF_stub* m = get_Matrix_now();
    f32 c = cosf(angle);
    f32 s = sinf(angle);

    if (mode == 0) { /* LOAD */
        mtxf_identity(m);
        m->mf[0][0] = c;  m->mf[0][2] = -s;
        m->mf[2][0] = s;  m->mf[2][2] = c;
    } else { /* MULT */
        for (int i = 0; i < 4; i++) {
            f32 x = m->mf[i][0];
            f32 z = m->mf[i][2];
            m->mf[i][0] = x * c + z * s;
            m->mf[i][2] = -x * s + z * c;
        }
    }
}

void Matrix_RotateZ(f32 angle, int mode) {
    MtxF_stub* m = get_Matrix_now();
    f32 c = cosf(angle);
    f32 s = sinf(angle);

    if (mode == 0) { /* LOAD */
        mtxf_identity(m);
        m->mf[0][0] = c;  m->mf[0][1] = s;
        m->mf[1][0] = -s; m->mf[1][1] = c;
    } else { /* MULT */
        for (int i = 0; i < 4; i++) {
            f32 x = m->mf[i][0];
            f32 y = m->mf[i][1];
            m->mf[i][0] = x * c - y * s;
            m->mf[i][1] = x * s + y * c;
        }
    }
}

void Matrix_rotateXYZ(s16 x, s16 y, s16 z, int mode) {
    /* Convert s16 angle to radians (0-65535 = 0-2π) */
    f32 ax = x * (3.14159265f / 32768.0f);
    f32 ay = y * (3.14159265f / 32768.0f);
    f32 az = z * (3.14159265f / 32768.0f);

    if (mode == 0) { /* LOAD - apply all rotations from identity */
        Matrix_RotateZ(az, 0);  /* Load Z rotation */
        Matrix_RotateY(ay, 1);  /* Multiply by Y rotation */
        Matrix_RotateX(ax, 1);  /* Multiply by X rotation */
    } else { /* MULT */
        Matrix_RotateZ(az, 1);
        Matrix_RotateY(ay, 1);
        Matrix_RotateX(ax, 1);
    }
}
void Matrix_softcv3_load(void* v) { (void)v; }
void Matrix_softcv3_mult(void* v) { (void)v; }

/* Convert float matrix to N64 fixed-point using guMtxF2L algorithm */
static void MtxF_to_Mtx_stub(MtxF_stub* src, Mtx* dest) {
    s32* ai = (s32*)&dest->m[0][0];
    s32* af = (s32*)&dest->m[2][0];

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 2; j++) {
            s32 e1 = (s32)(src->mf[i][j*2] * 65536.0f);
            s32 e2 = (s32)(src->mf[i][j*2+1] * 65536.0f);
            *(ai++) = (e1 & 0xFFFF0000) | ((e2 >> 16) & 0xFFFF);
            *(af++) = ((e1 << 16) & 0xFFFF0000) | (e2 & 0xFFFF);
        }
    }
}
/* Convert current Matrix_now to N64 fixed-point format */
static int mtx_to_mtx_debug = 0;
Mtx* _Matrix_to_Mtx(Mtx* dest) {
    if (!dest) return NULL;
    if (mtx_to_mtx_debug < 5) {
        MtxF_stub* src = get_Matrix_now();
        printf("[_Matrix_to_Mtx] dest=%p lower32=0x%08X, Matrix_now row0=(%.2f,%.2f,%.2f,%.2f)\n",
               (void*)dest, (u32)(uintptr_t)dest,
               src->mf[0][0], src->mf[0][1], src->mf[0][2], src->mf[0][3]);
        fflush(stdout);
        mtx_to_mtx_debug++;
    }
    MtxF_to_Mtx_stub(get_Matrix_now(), dest);
    return dest;
}

/* Allocate matrix from graph buffer and fill with identity */
static int mtx_new_debug = 0;
Mtx* _Matrix_to_Mtx_new(GRAPH* graph) {
    Mtx* mtx = GRAPH_ALLOC(graph, sizeof(Mtx));
    if (mtx_new_debug < 5) {
        MtxF_stub* src = get_Matrix_now();
        printf("[_Matrix_to_Mtx_new] mtx=%p, Matrix_now row0=(%.2f,%.2f,%.2f,%.2f)\n",
               (void*)mtx, src->mf[0][0], src->mf[0][1], src->mf[0][2], src->mf[0][3]);
        fflush(stdout);
        mtx_new_debug++;
    }
    return _Matrix_to_Mtx(mtx);
}
void MtxF_clear(void* mf) { (void)mf; }
void Skin_Matrix_PrjMulVector(void* mf, void* in, void* out, f32* w) { (void)mf; (void)in; (void)out; (void)w; }
void sMath_RotateX(f32 angle, void* v) { (void)angle; (void)v; }
void sMath_RotateZ(f32 angle, void* v) { (void)angle; (void)v; }

/* ============================================================================
 * guTranslateF/guScaleF - libultra matrix utilities
 * ============================================================================ */
void guTranslateF(f32 mf[4][4], f32 x, f32 y, f32 z) {
    memset(mf, 0, 64);
    mf[0][0] = mf[1][1] = mf[2][2] = mf[3][3] = 1.0f;
    mf[3][0] = x; mf[3][1] = y; mf[3][2] = z;
}
void guScaleF(f32 mf[4][4], f32 x, f32 y, f32 z) {
    memset(mf, 0, 64);
    mf[0][0] = x; mf[1][1] = y; mf[2][2] = z; mf[3][3] = 1.0f;
}

/* ============================================================================
 * Math3D functions
 * ============================================================================ */
f32 Math3DLength(const void* v) { (void)v; return 0.0f; }
void Math3DPlane(const void* a, const void* b, const void* c, void* plane) {
    (void)a; (void)b; (void)c; (void)plane;
}
void Math3d_normalizeXyz_t(void* v) { (void)v; }
void Math3DInDivPos2(void* out, const void* a, const void* b, f32 t) { (void)out; (void)a; (void)b; (void)t; }
int Math3D_sphereCrossSphere_cl(void* a, void* b, void* c) { (void)a; (void)b; (void)c; return 0; }
int Math3D_sphereCrossTriangle3_cp(void* a, void* b, void* c, void* d, void* e) {
    (void)a; (void)b; (void)c; (void)d; (void)e; return 0;
}
int Math3D_sphereVsPipe_cl(void* a, void* b, void* c, void* d) { (void)a; (void)b; (void)c; (void)d; return 0; }
int Math3D_pipeVsPipe_cl(void* a, void* b, void* c, void* d, void* e) {
    (void)a; (void)b; (void)c; (void)d; (void)e; return 0;
}
int Math3D_pipeCrossTriangle_cp(void* a, void* b, void* c, void* d, void* e, void* f) {
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; return 0;
}
f32 facos(f32 x) { return acosf(x); }

/* ============================================================================
 * Screen dimensions
 * ============================================================================ */
int ScreenWidth = 640;
int ScreenHeight = 480;

/* ============================================================================
 * Global game functions
 * ============================================================================ */
int _Game_play_isPause(void* play) { (void)play; return 0; }
void Game_play_Projection_Trans(void* play, void* world, void* screen) { (void)play; (void)world; (void)screen; }
void* g_player_actor_ptr = NULL;  /* Set by m_actor.c when player spawns */

void* get_player_actor_withoutCheck(void* play) {
    (void)play;
    return g_player_actor_ptr;
}

/* ============================================================================
 * Memory functions
 * ============================================================================ */
void mem_copy(u8* dst, const u8* src, u32 size) { memcpy(dst, src, size); }
void mem_clear(u8* dst, size_t size, u8 val) { memset(dst, val, size); }
void* zelda_malloc(u32 size) { return malloc(size); }
void* zelda_malloc_r(u32 size) { return malloc(size); }
void zelda_free(void* ptr) { free(ptr); }
void msleep(int ms) { (void)ms; }

/* ============================================================================
 * THA (TwoHeadArena)
 * ============================================================================ */
void* THA_allocAlign(TwoHeadArena* tha, size_t size, int mask) { (void)tha; (void)mask; return malloc(size); }

/* ============================================================================
 * GBI allocation
 * ============================================================================ */
void* gfxalloc(void** gfxp, u32 size) {
    void* result = *gfxp;
    *gfxp = (u8*)*gfxp + size;
    return result;
}

/* ============================================================================
 * Lighting functions
 * ============================================================================ */
void Global_light_read(void* glight, void* play, int index) { (void)glight; (void)play; (void)index; }
void LightsN_list_check(void* lights, void* gfxCtx, void* play) { (void)lights; (void)gfxCtx; (void)play; }
void LightsN_disp(void* lights, void** gfxp, void* gfxCtx) { (void)lights; (void)gfxp; (void)gfxCtx; }
void LightsN_disp_BG(void* lights, void** gfxp, void* gfxCtx) { (void)lights; (void)gfxp; (void)gfxCtx; }
void* Global_light_list_new(void* glight, int type) { (void)glight; (void)type; return NULL; }
void Global_light_list_delete(void* glight, void* node) { (void)glight; (void)node; }
void Light_diffuse_ct(void* light, u8 r, u8 g, u8 b, s16 x, s16 y, s16 z) {
    (void)light; (void)r; (void)g; (void)b; (void)x; (void)y; (void)z;
}
void Light_point_ct(void* light, s16 x, s16 y, s16 z, u8 r, u8 g, u8 b, s16 radius) {
    (void)light; (void)x; (void)y; (void)z; (void)r; (void)g; (void)b; (void)radius;
}
void Light_list_point_draw(void* lights, void* gfxp, void* gfxCtx) { (void)lights; (void)gfxp; (void)gfxCtx; }

/* ============================================================================
 * NPC functions
 * ============================================================================ */
void* mNpc_GetAnimalInfoP(s16 idx) { (void)idx; return NULL; }
void mNpc_SetNpcinfo(void* actor, s8 npc_info_idx) { (void)actor; (void)npc_info_idx; }
s16 mNpc_SearchAnimalinfo(s16 id) { (void)id; return -1; }
int mNpc_CheckFreeAnimalPersonalID(void* id) { (void)id; return 0; }
void mNpc_RegistEventNpc(void* info, int idx) { (void)info; (void)idx; }
void mNpc_RegistMaskNpc(s16 id, int type) { (void)id; (void)type; }

/* ============================================================================
 * Player functions
 * ============================================================================ */
void mPlib_Object_Exchange_keep_new_PlayerMdl(void* ex) { (void)ex; }
void mPlib_Object_Exchange_keep_new_PlayerTex(void* ex, int idx) { (void)ex; (void)idx; }
void mPlib_Object_Exchange_keep_new_PlayerPallet(void* ex, int idx) { (void)ex; (void)idx; }
void mPlib_Object_Exchange_keep_new_PlayerFaceTex(void* ex) { (void)ex; }
void mPlib_Object_Exchange_keep_new_PlayerFacePallet(void* ex, int idx) { (void)ex; (void)idx; }
void mPlib_Load_PlayerTexAndPallet(void* ex) { (void)ex; }
void mPlib_request_main_invade_type1(void* play) { (void)play; }
void mPlib_request_main_door_type1(void* play, void* door) { (void)play; (void)door; }
void mPlib_request_main_door_type2(void* play, void* door) { (void)play; (void)door; }
void mPlib_request_main_talk_type1(void* play, void* actor) { (void)play; (void)actor; }
void mPlib_request_main_talk_end_type1(void* play) { (void)play; }
void mPlib_request_main_demo_wait_from_submenu(void* play) { (void)play; }
int mPlib_check_request_main_outdoor_priority(void* play) { (void)play; return 1; }
int mPlib_check_request_main_wade_priority(void* play) { (void)play; return 1; }
int mPlib_check_request_main_demo_wade_priority(void* play) { (void)play; return 1; }
int mPlib_check_request_main_demo_geton_boat_wade_priority(void* play) { (void)play; return 1; }
int mPlib_check_request_main_door_type1_priority(void* play) { (void)play; return 1; }
int mPlib_check_request_main_door_type2_priority(void* play) { (void)play; return 1; }
int mPlib_check_request_main_speak_type1_priority(void* play) { (void)play; return 1; }
int mPlib_check_request_main_talk_type1_priority(void* play) { (void)play; return 1; }
void mPlib_strength_request_main_door_priority(void* play) { (void)play; }
int mPlib_get_player_actor_main_index(void* actor) { (void)actor; return 0; }
int mPlib_check_able_change_camera_normal_index(void* play) { (void)play; return 1; }
int mPlib_Check_able_force_speak_label(void* play, void* actor) { (void)play; (void)actor; return 0; }
int mPlib_able_submenu_type1(void* play) { (void)play; return 1; }
int mPlib_able_player_warp_forEvent(void* play) { (void)play; return 1; }
int mPlib_Get_unable_wade(void* play) { (void)play; return 0; }
void* mPlib_Get_address_able_display(void* play) { (void)play; return NULL; }
s16 mPlib_Get_itemNo_forWindow(void* play) { (void)play; return -1; }
void mPlib_SetData1_controller_data_for_title_demo(void) { }
void mPlib_SetData2_controller_data_for_title_demo(void) { }

/* ============================================================================
 * Field functions
 * ============================================================================ */
void mFI_FieldMove(void* play) { (void)play; }
void mFM_SetFieldInitData(int a, int b) { (void)a; (void)b; }
int mFI_GetFieldId(void) { return 0; }
int mFI_GetBlockWidth(void) { return 16; }
int mFI_GetBlockHeight(void) { return 16; }
int mFI_GetBlockXMax(void) { return 5; }
int mFI_GetBlockZMax(void) { return 5; }
int mFI_Wpos2BlockNum(int* bx, int* bz, xyz_stub wpos) {
    /* Return center block coordinates */
    if (bx) *bx = 2;  /* Default to center-ish block */
    if (bz) *bz = 2;
    (void)wpos;
    return 1;  /* Success */
}
void mFI_Wpos2UtNum(f32 x, f32 z, int* ux, int* uz) { (void)x; (void)z; *ux = 0; *uz = 0; }
void mFI_Wpos2UtNum_inBlock(f32 x, f32 z, int* ux, int* uz) { (void)x; (void)z; *ux = 0; *uz = 0; }
void mFI_Wpos2BkandUtNuminBlock(f32 x, f32 z, int* bk, int* ux, int* uz) { (void)x; (void)z; *bk = 0; *ux = 0; *uz = 0; }
int mFI_BkNum2WposXZ(f32* wpos_x, f32* wpos_z, int bx, int bz) {
    /* Convert block coords to world position */
    if (wpos_x) *wpos_x = (f32)(bx * 640);  /* 640 units per block */
    if (wpos_z) *wpos_z = (f32)(bz * 640);
    return 1;
}
int mFI_GetNextBlockNum(int bk, int dir) { (void)bk; (void)dir; return 0; }
int mFI_BlockKind2BkNum(int kind) { (void)kind; return 0; }
int mFI_CheckBlockKind_OR(int bk, int kinds) { (void)bk; (void)kinds; return 0; }
int mFI_CheckFieldData(void) { return 0; }
int mFI_CheckInIsland(void) { return 0; }
int mFI_CheckInJustIslandOutdoor(void) { return 0; }
int mFI_CheckPlayerWade(void) { return 0; }
void mFI_LposInBKtoWpos(void* wpos, void* lpos, int bk) { (void)wpos; (void)lpos; (void)bk; }
int mFI_SearchFGInBlock(void* data, int bk, int type) { (void)data; (void)bk; (void)type; return -1; }
int mFI_search_unit_around(void* data, int ut_x, int ut_z, int radius, void* callback) {
    (void)data; (void)ut_x; (void)ut_z; (void)radius; (void)callback; return 0;
}
void mFI_SetBearActor(void* actor) { (void)actor; }
void mFI_SetFG_common(int ut_x, int ut_z, int fg_id, int type) { (void)ut_x; (void)ut_z; (void)fg_id; (void)type; }
void mFI_SetFGStructure_common(int a, int b, int c, int d) { (void)a; (void)b; (void)c; (void)d; }

/* ============================================================================
 * Collision/BG functions
 * ============================================================================ */
int mCoBG_CheckCliffAttr(int attr) { (void)attr; return 0; }
f32 mCoBG_GetBgY_OnlyCenter_FromWpos2(void* pos, int* hit) { (void)pos; if(hit) *hit = 0; return 0.0f; }
int mCoBG_SearchWaterLimitDistN(void* pos, f32* dist) { (void)pos; (void)dist; return 0; }
int mCoBG_UtNum2BgAttr(int ut_x, int ut_z) { (void)ut_x; (void)ut_z; return 0; }
f32 mCoBG_UtNum2UtCenterY(int ut_x, int ut_z) { (void)ut_x; (void)ut_z; return 0.0f; }

/* ============================================================================
 * Land check functions
 * ============================================================================ */
int mLd_CheckCmpLand(void) { return 0; }
int mLd_CheckCmpLandName(void) { return 0; }

/* ============================================================================
 * Scene functions
 * ============================================================================ */
void mScn_ObtainMyRoomBank(void* ex, void* common) { (void)ex; (void)common; }
void mScn_ObtainCarpetBank(void* ex, void* common) { (void)ex; (void)common; }

/* ============================================================================
 * Room functions
 * ============================================================================ */
void mRmTp_GetEntranceBasePosition(void* pos) { (void)pos; }
int mRmTp_GetNowSceneLightSwitchIndex(void) { return 0; }
int mRmTp_Index2LightSwitchStatus(int idx) { (void)idx; return 0; }
void mRmTp_IndexLightSwitchON(int idx) { (void)idx; }
void mRmTp_IndexLightSwitchOFF(int idx) { (void)idx; }
void mRmTp_NowSceneLightSwitchON(void) { }
void mRmTp_NowSceneLightSwitchOFF(void) { }
void mRmTp_PleaseDrawLightSwitch(void* play) { (void)play; }
void mRmTp_SetDefaultLightSwitchData(void) { }

/* ============================================================================
 * Submenu
 * ============================================================================ */
void mSM_menu_ovl_init(void) { }

/* ============================================================================
 * Misc game functions
 * ============================================================================ */
void mBI_move(void* play) { (void)play; }
void mTM_check_renew_time(void) { }
void mTM_off_renew_time(void) { }
/* REMOVED: aWeather_ChangingWeather - now in ac_weather.c */
int mCD_calendar_wellcome_on(void) { return 0; }
void mMl_clear_mail(void* mail) { (void)mail; }
u32 mMl_strlen(const char* str) { return (u32)strlen(str); }
void* mMld_GetMelody(int idx) { (void)idx; return NULL; }
void mIN_copy_name_str(char* dst, const char* src) { if(dst && src) strcpy(dst, src); }
void mPr_ClearPersonalID(void* id) { (void)id; }
void mPr_ClearAnyPersonalID(void* id) { (void)id; }
int mPr_NullCheckPersonalID(void* id) { (void)id; return 1; }
int mPr_GetPossessionItemSumWithCond(void* id, int cond) { (void)id; (void)cond; return 0; }
int mQst_CheckLimitbyPossessionIdx(void* play, int idx) { (void)play; (void)idx; return 0; }
int mSP_GetShopCloseTime_Bgm(void) { return 22; }
int mSP_SelectRandomItem_New(void* item, int type) { (void)item; (void)type; return 0; }
int mFAs_GetFieldRank(void) { return 0; }
void mFAs_SetFieldRank(int rank) { (void)rank; }
void mAGrw_OrderSetHaniwa(void* play) { (void)play; }
void mSC_LightHouse_Event_Start(void* play) { (void)play; }
int mSC_LightHouse_travel_check(void) { return 0; }
void mNPS_schedule_manager(void* play) { (void)play; }
int mNPS_get_schedule_area(void* info) { (void)info; return 0; }

/* ============================================================================
 * Font/Text
 * ============================================================================ */
void mFont_Main_start(void) { }
void mFont_SetLineStrings(void** gfxp, const char* str, int x, int y, u8 color, u8 alpha) {
    (void)gfxp; (void)str; (void)x; (void)y; (void)color; (void)alpha;
}
int mFont_GetStringWidth(const char* str, int len, int spacing) { (void)str; (void)spacing; return len * 8; }
void mFont_CulcOrthoMatrix(void) { }
void mFont_UnintToString(char* buf, int bufsize, u32 value, int digits, int leading_zeros, int align) {
    (void)buf; (void)bufsize; (void)value; (void)digits; (void)leading_zeros; (void)align;
}

/* ============================================================================
 * GFX print
 * ============================================================================ */
void gfxprint_color(void* gfxprint, int r, int g, int b, int a) { (void)gfxprint; (void)r; (void)g; (void)b; (void)a; }
void gfxprint_locate8x8(void* gfxprint, int x, int y) { (void)gfxprint; (void)x; (void)y; }
void gfxprint_printf(void* gfxprint, const char* fmt, ...) { (void)gfxprint; (void)fmt; }

/* ============================================================================
 * RTC functions
 * ============================================================================ */
void lbRTC_GetTime(void* rtc) { (void)rtc; }
void lbRTC_TimeCopy(void* dst, void* src) { (void)dst; (void)src; }
int lbRTC_IsEqualDate(void* a, void* b) { (void)a; (void)b; return 0; }
int lbRTC_Weekly_day(int y, int m, int d) { (void)y; (void)m; (void)d; return 0; }
int lbRTC_Sub_DD(void* a, void* b) { (void)a; (void)b; return 0; }
int lbRTC_Sub_mm(void* a, void* b) { (void)a; (void)b; return 0; }
int lbRTC_Sub_ss(void* a, void* b) { (void)a; (void)b; return 0; }
void lbRTC_Add_DD(void* rtc, int days) { (void)rtc; (void)days; }
int lbRk_VernalEquinoxDay(int year) { (void)year; return 80; }
int lbRk_AutumnalEquinoxDay(int year) { (void)year; return 266; }
int lbRk_HarvestMoonDay(int year) { (void)year; return 258; }

/* ============================================================================
 * Skeleton/Animation
 * ============================================================================ */
void cKF_SkeletonInfo_R_ct(void* info, void* skeleton, void* callback, int num_limbs, void* work, void* morph) {
    (void)info; (void)skeleton; (void)callback; (void)num_limbs; (void)work; (void)morph;
}
void cKF_SkeletonInfo_R_dt(void* info) { (void)info; }
void cKF_SkeletonInfo_R_init(void* info, void* skeleton, void* anim, f32 start, f32 end, f32 frame, f32 speed, f32 morph, int mode, void* callback) {
    (void)info; (void)skeleton; (void)anim; (void)start; (void)end; (void)frame; (void)speed; (void)morph; (void)mode; (void)callback;
}
int cKF_SkeletonInfo_R_play(void* info) { (void)info; return 1; }  /* Return TRUE = animation done */
void cKF_SkeletonInfo_subRotInterpolation(void* info) { (void)info; }
f32 cKF_HermitCalc(f32 t, f32 p0, f32 m0, f32 p1, f32 m1) { (void)t; (void)p0; (void)m0; (void)p1; (void)m1; return 0.0f; }
void cKF_Si3_draw_SV_R_child(void* play, void* info, void* mtx, void* callback, void* arg) {
    (void)play; (void)info; (void)mtx; (void)callback; (void)arg;
}

/* ============================================================================
 * JSystem wrappers
 * ============================================================================ */
void* _JW_GetResourceAram(u32 resType, u16 resId) { (void)resType; (void)resId; return NULL; }
void JW_setClearColor(int r, int g, int b, int a) { (void)r; (void)g; (void)b; (void)a; }
int OSGetSoundMode(void) { return 0; }

/* ============================================================================
 * Actor DLL tables - Now implemented in m_actor_dlftbls_pc.c
 * ============================================================================ */
/* Removed: actor_dlftbls, actor_dlftbls_init, actor_dlftbls_cleanup */
int chkTrigger(u16 mask) { (void)mask; return 0; }

/* ============================================================================
 * Audio stubs - Na_* functions (jaudio NES audio system)
 * ============================================================================ */
void Na_GameFrame(void) { }
void Na_BgmStart(int bgm_id) { (void)bgm_id; }
void Na_BgmStop(void) { }
void Na_BGMVolume(f32 vol) { (void)vol; }
void Na_BgmCrossfadeStart(int bgm_id, int frames) { (void)bgm_id; (void)frames; }
int Na_BgmFadeoutCheck(void) { return 0; }
int Na_SeFadeoutCheck(void) { return 0; }
void Na_Inst(u32 inst_id) { (void)inst_id; }
int Na_InstCountGet(void) { return 0; }
void Na_FurnitureInst(void* ftr) { (void)ftr; }
void Na_FurnitureInstPos(void* ftr, void* pos) { (void)ftr; (void)pos; }
void Na_OngenPos(void* pos, int id) { (void)pos; (void)id; }
void Na_OngenTrgStart(int id) { (void)id; }
void Na_OngenTrgStartSpeed(int id, f32 speed) { (void)id; (void)speed; }
void Na_SysTrgStart(u32 id) { (void)id; }
void Na_SysLevStart(u32 id) { (void)id; }
void Na_SysLevStop(u32 id) { (void)id; }
void Na_TrgSeEcho(int id, int echo) { (void)id; (void)echo; }
void Na_LevSeEcho(int id) { (void)id; }
void Na_MDPlayerPos(void* pos) { (void)pos; }
void Na_FloorTrgStart(int id) { (void)id; }
void Na_PlyWalkSe(int se_id, int param) { (void)se_id; (void)param; }
void Na_PlyWalkSeRoom(int se_id, int param) { (void)se_id; (void)param; }
void Na_NpcWalkSe(int se_id, int npc_type) { (void)se_id; (void)npc_type; }
void Na_NpcWalkSeRoom(int se_id, int npc_type) { (void)se_id; (void)npc_type; }
void Na_VoiceSe(int se_id, int voice) { (void)se_id; (void)voice; }
void Na_MessageStatus(int status) { (void)status; }
void Na_MessageSpeed(int speed) { (void)speed; }
int Na_MessageSpeedGet(void) { return 1; }
void Na_RhythmStart(void* data) { (void)data; }
void Na_RhythmStop(void) { }
void Na_RhythmAllStop(void) { }
void Na_RhythmPos(void* pos) { (void)pos; }
void Na_SetRhythmInfo(void* info) { (void)info; }
void* Na_GetRhythmInfo(void) { return NULL; }
int Na_GetRhythmDelay(void) { return 0; }
int Na_GetRhythmAnimCounter(void) { return 0; }
void Na_Tenki(int weather) { (void)weather; }
void Na_RoomType(int type) { (void)type; }
void Na_Museum(int mode) { (void)mode; }
void Na_SceneMode(int mode) { (void)mode; }
void Na_RoomIncectPos(void* pos, int id) { (void)pos; (void)id; }
void Na_Pause(int pause) { (void)pause; }
void Na_SetOutMode(int mode) { (void)mode; }
void Na_SetVoiceMode(int mode) { (void)mode; }
void Na_SpecChange(int spec) { (void)spec; }
void Na_SoftReset(void) { }
void Na_SubGameStart(int id) { (void)id; }
void Na_SubGameEnd(void) { }
void Na_SubGameOK(void) { }
void Na_KishaStatusTrg(int status) { (void)status; }
void Na_KishaStatusLevel(int level) { (void)level; }
int Na_GetRadioCounter(void) { return 0; }
int Na_GetKappaCounter(void) { return 0; }
int Na_GetStaffRollInfo(void) { return 0; }
void Na_TTKK_ARM(int mode) { (void)mode; }
int Na_GetSoundFrameCounter(void) { return 0; }
void Na_kazagurumaLevel(int level) { (void)level; }
void Na_PlayerStatusLevel(int level) { (void)level; }

/* ============================================================================
 * Model data stubs (dummy Gfx arrays for graphics models referenced by code)
 * ============================================================================ */
u64 ef_wipe1_modelT[2] = {0};
u64 ef_wipe2_modelT[2] = {0};
u64 ef_wipe3_modelT[2] = {0};
u64 camera_model[2] = {0};
u64 darrow_model[2] = {0};
u64 cam_win_cT_model[2] = {0};
u64 cam_win_mojiT_model[2] = {0};
u64 cam_win_winT_model[2] = {0};
u64 cam_win_yajirushi_model[2] = {0};
u64 clk_win_ampmT_model[2] = {0};
u64 clk_win_maru2T_model[2] = {0};
u64 clk_win_maruT_model[2] = {0};
u64 clk_win_youbiT_model[2] = {0};
u64 mny_win_beruT_model[2] = {0};
u64 mny_win_mojiT_model[2] = {0};
u64 mny_win_ueT_model[2] = {0};
u64 elc_win_moji2T_model[2] = {0};
u64 elc_win_moji_model[2] = {0};
u64 elc_win_winT_model[2] = {0};
u64 elc_win_zT_model[2] = {0};
u64 fki_win_w1T_model[2] = {0};
u64 fki_win_w2T_model[2] = {0};
u64 fki_win_w3T_model[2] = {0};
u64 fki_win_w4_model[2] = {0};

/* Texture stubs */
u8 clk_win_am_tex_rgb_ia8[8] = {0};
u8 clk_win_pm_tex_rgb_ia8[8] = {0};
u8 clk_win_sun_tex_rgb_ia8[8] = {0};
u8 clk_win_mon_tex_rgb_ia8[8] = {0};
u8 clk_win_tue_tex_rgb_ia8[8] = {0};
u8 clk_win_wed_tex_rgb_ia8[8] = {0};
u8 clk_win_thu_tex_rgb_ia8[8] = {0};
u8 clk_win_fri_tex_rgb_ia8[8] = {0};
u8 clk_win_sat_tex_rgb_ia8[8] = {0};
u8 RCP_debug_texture_16x16_8[256] = {0};
u8 clk_win_suuji1_TA_tex_txt[8] = {0};
u8 clk_win_suuji2_TA_tex_txt[8] = {0};
u8 clk_win_suuji3_TA_tex_txt[8] = {0};
u8 clk_win_suuji4_TA_tex_txt[8] = {0};
u8 clk_win_suuji5_TA_tex_txt[8] = {0};
u8 clk_win_suuji6_TA_tex_txt[8] = {0};
u8 clk_win_suuji7_TA_tex_txt[8] = {0};
u8 clk_win_suuji8_TA_tex_txt[8] = {0};
u8 clk_win_suuji9_TA_tex_txt[8] = {0};
u8 clk_win_suuji10_TA_tex_txt[8] = {0};
u8 clk_win_suuji11_TA_tex_txt[8] = {0};
u8 clk_win_suuji12_TA_tex_txt[8] = {0};
u8 clk_win_suuji13_TA_tex_txt[8] = {0};
u8 clk_win_suuji14_TA_tex_txt[8] = {0};
u8 clk_win_suuji15_TA_tex_txt[8] = {0};
u8 clk_win_suuji16_TA_tex_txt[8] = {0};
u8 clk_win_suuji17_TA_tex_txt[8] = {0};
u8 clk_win_suuji18_TA_tex_txt[8] = {0};
u8 clk_win_suuji19_TA_tex_txt[8] = {0};
u8 clk_win_suuji20_TA_tex_txt[8] = {0};
u8 clk_win_suuji21_TA_tex_txt[8] = {0};
u8 clk_win_suuji22_TA_tex_txt[8] = {0};
u8 clk_win_suuji23_TA_tex_txt[8] = {0};
u8 clk_win_suuji24_TA_tex_txt[8] = {0};
u8 clk_win_suuji25_TA_tex_txt[8] = {0};
u8 clk_win_suuji26_TA_tex_txt[8] = {0};
u8 clk_win_suuji27_TA_tex_txt[8] = {0};
u8 clk_win_suuji28_TA_tex_txt[8] = {0};
u8 clk_win_suuji29_TA_tex_txt[8] = {0};
u8 clk_win_suuji30_TA_tex_txt[8] = {0};
u8 clk_win_suuji31_TA_tex_txt[8] = {0};
u8 clk_win_jikan_TA_tex_txt[8] = {0};
u8 clk_win_jikan0_TA_tex_txt[8] = {0};
u8 clk_win_jikan1_TA_tex_txt[8] = {0};
u8 clk_win_jikan2_TA_tex_txt[8] = {0};
u8 clk_win_jikan3_TA_tex_txt[8] = {0};
u8 clk_win_jikan4_TA_tex_txt[8] = {0};
u8 clk_win_jikan5_TA_tex_txt[8] = {0};
u8 clk_win_jikan6_TA_tex_txt[8] = {0};
u8 clk_win_jikan7_TA_tex_txt[8] = {0};
u8 clk_win_jikan8_TA_tex_txt[8] = {0};
u8 clk_win_jikan9_TA_tex_txt[8] = {0};
u8 no_txt[8] = {0};
u8 np_txt[8] = {0};
u8 nt_txt[8] = {0};
u8 nx_txt[8] = {0};

/* Animation data stubs */
void* pact0_head_table = NULL;
void* pact0_key_data = NULL;
void* pact1_head_table = NULL;
void* pact1_key_data = NULL;
void* pact2_head_table = NULL;
void* pact2_key_data = NULL;
void* pact3_head_table = NULL;
void* pact3_key_data = NULL;
void* pact4_head_table = NULL;
void* pact4_key_data = NULL;
void* cKF_ba_r_clk_hiniti = NULL;
void* cKF_ba_r_clk_jikan = NULL;
void* cKF_bs_r_clk_hiniti = NULL;
void* cKF_bs_r_clk_jikan = NULL;

/* Window modes */
int clk_win_mode = 0;
int fki_win_mode = 0;

/* ============================================================================
 * Field info stubs (ac_field_draw.c dependencies)
 * ============================================================================ */

/* Structure for field draw info */
typedef struct { int dummy; } mFM_field_draw_info_stub;
typedef struct { u16 kind; xyz_stub pos; } mFM_bg_sound_source_stub;
typedef struct { int type; int data; void* scroll_data; } EVW_ANIME_DATA_stub;

/* Field info functions */
/* Declaration of test terrain function (from test_terrain.c) */
extern void* test_terrain_get_display_list(int bx, int bz);

void* mFI_BGDisplayListTop(void) { return NULL; }
void* mFI_GetBGDisplayListRom(int bx, int bz) {
    /* Return test terrain display list for PC port testing */
    return test_terrain_get_display_list(bx, bz);
}
void* mFI_GetBGDisplayListRom_XLU(int bx, int bz) { (void)bx; (void)bz; return NULL; }
void* mFI_GetBGTexAnimInfo(void* frame_count, int bx, int bz) {
    (void)frame_count; (void)bx; (void)bz;
    return NULL;
}
void* mFI_GetSoundSourcePBlockNum(int bx, int bz) { (void)bx; (void)bz; return NULL; }
f32 mFI_BkNum2BaseHeight(int bx, int bz) { (void)bx; (void)bz; return 0.0f; }
u32 mFI_BkNum2BlockKind(int bx, int bz) { (void)bx; (void)bz; return 0; }
int mFI_BkNum2BlockType(int bx, int bz) { (void)bx; (void)bz; return 0; }

/* Collision BG functions */
f32 mCoBG_GetBgY_OnlyCenter_FromWpos(xyz_stub pos, f32 offset) {
    (void)pos; (void)offset;
    return 0.0f;
}
void mCoBG_WaveCos2BgCheck(f32 wave_cos) { (void)wave_cos; }

/* Random field functions */
int mRF_SearchPond(int* px, int* pz, int bx, int bz) {
    (void)px; (void)pz; (void)bx; (void)bz;
    return 0;
}

/* Environment animation */
void Evw_Anime_Set(void* play, void* data) { (void)play; (void)data; }

/* Math table functions */
f32 cosf_table(f32 angle) { return cosf(angle); }

/* ============================================================================
 * Player actor stubs (from m_player.c)
 * These are the actual player implementation functions called by m_player_call.c
 * For now, stub them to allow player actor to spawn without full implementation
 * ============================================================================ */

/* Forward declaration for ACTOR/GAME types */
struct actor_s;
struct game_s;

void Player_actor_ct(struct actor_s* actor, struct game_s* game) {
    (void)actor; (void)game;
    printf("[PLAYER] Player_actor_ct stub called\n");
}

void Player_actor_dt(struct actor_s* actor, struct game_s* game) {
    (void)actor; (void)game;
    printf("[PLAYER] Player_actor_dt stub called\n");
}

void Player_actor_move(struct actor_s* actor, struct game_s* game) {
    (void)actor; (void)game;
    /* Don't spam - this is called every frame */
}

void Player_actor_draw(struct actor_s* actor, struct game_s* game) {
    (void)actor; (void)game;
    /* Don't spam - this is called every frame */
}

/* ============================================================================
 * TITLE_DEMO actor dependencies
 * Stubs for ac_effectbg.c and ac_animal_logo.c
 * ============================================================================ */

/* Field info functions */
int mFI_GetFieldPal(void) { return 0; }
int mFI_GetUnitFG(int ux, int uz) { (void)ux; (void)uz; return 0; }

/* Random functions */
f32 fqrand2(void) { return 0.5f; }

/* Keyframe animation */
void cKF_SkeletonInfo_R_init_standard_stop(void* info, void* anim, void* callback) {
    (void)info; (void)anim; (void)callback;
}
void cKF_Si3_draw_R_SV(void* play, void* info, void* mtx, void* callback, void* arg) {
    (void)play; (void)info; (void)mtx; (void)callback; (void)arg;
}

/* Collision BG with angle */
f32 mCoBG_GetBgY_AngleS_FromWpos(void* angles, xyz_stub pos, f32 offset) {
    (void)angles; (void)pos; (void)offset;
    return 0.0f;
}

/* Matrix functions */
void Matrix_Position_VecX(void* mtx, void* vec) { (void)mtx; (void)vec; }

/* RTC functions */
int lbRTC_IsAbnormal(void) { return 0; }
void lbRTC_Sampling(void) { }

/* Time/Calendar functions */
void mTM_rtcTime_default_code(void) { }
void mTM_clear_renew_is(void) { }
int mTM_rtcTime_limit_check(void) { return 0; }

/* Card/Save functions */
int mCD_LoadLand(void) { return 1; }  /* Return success */
void mCD_set_aram_save_data(void) { }

/* NPC functions */
void* mNpc_GetInAnimalP(int idx) { (void)idx; return NULL; }
void mNpc_ClearAnimalInfo(void* animal) { (void)animal; }

/* Famicom/GBA */
int famicom_mount_archive_end_check(void) { return 1; }  /* Return done */

/* Land/Load functions */
int mLd_CheckStartFlag(void) { return 1; }  /* Return ready */

/* Font matrix functions */
void mFont_SetMatrix(void* gfxp) { (void)gfxp; }
void mFont_SetMode(void* gfxp, int mode) { (void)gfxp; (void)mode; }
void mFont_UnSetMatrix(void* gfxp) { (void)gfxp; }

/* ============================================================================
 * Structure Actor stubs (ac_structure.c dependencies)
 * ============================================================================ */

/* Structure palette address tables - MOVED TO REAL FILES:
 * src/data/model/structure/structure_pal.c (tables)
 * src/data/model/structure/palette/structure_pal_data.c (data) */

/* ============================================================================
 * Birth Control Actor stubs (ac_birth_control.c dependencies)
 * ============================================================================ */

/* GBA connection functions (GBA-to-GameCube link, not needed for PC port) */
void mGcgba_InitVar(void) { }
int mGcgba_ConnectEnabled(void) { return 0; }  /* Return failed/no GBA connected */

/* Field info - block data access */
typedef struct {
    int born_actor;
} mFI_field_data_info_t;
static mFI_field_data_info_t g_fdinfo_data = {0};
mFI_field_data_info_t* g_fdinfo = &g_fdinfo_data;

int mFI_ActorisBorn(void) { return 0; }  /* No actors born yet */
void* mFI_MoveActorListDma(int bx, int bz) { (void)bx; (void)bz; return NULL; }
void mNpc_AddActor_inBlock(void* mv_actor_list, int bx, int bz) { (void)mv_actor_list; (void)bx; (void)bz; }
u16 mFI_GetMoveActorBitData(int bx, int bz) { (void)bx; (void)bz; return 0; }
void mFI_SetMoveActorBitData(int bx, int bz, u16 data) { (void)bx; (void)bz; (void)data; }
void* mFI_UtNum2UtFG(int ut_num, int bz) { (void)ut_num; (void)bz; return NULL; }
int mFI_CheckBeforeScenePerpetual(void) { return 0; }
int mFI_GetClimate(void) { return 0; }  /* Default climate */
u32 mFI_CheckPlayerBlockInfo(void) { return 0; }

/* ============================================================================
 * Weather Actor stubs (ac_weather.c dependencies)
 * ============================================================================ */

/* Player library functions */
int mPlib_check_player_open_umbrella(void* game) { (void)game; return 0; }

/* ============================================================================
 * Weather effect asset stubs (rain/snow/sakura/leaf particle effects)
 * ============================================================================ */

/* Rain effects */
void ef_ame02_setmode(void* gfxp) { (void)gfxp; }
u64 ef_ame02_00_modelT[2] = {0};
u64 ef_ame02_01_modelT[2] = {0};
u64 ef_ame02_02_modelT[2] = {0};
u64 ef_ame02_03_modelT[2] = {0};
u64 ef_ame02_04_modelT[2] = {0};

/* Snow effects */
void ef_yuki01_setmode(void* gfxp) { (void)gfxp; }
u64 ef_yuki01_00_model[2] = {0};

/* Cherry blossom (sakura) effects */
void ef_hanabira01_00_setmode(void* gfxp) { (void)gfxp; }
u64 ef_hanabira01_00_modelT[2] = {0};

/* Leaf effects */
void ef_otiba01_setmode(void* gfxp) { (void)gfxp; }
u64 ef_otiba01_00_modelT[2] = {0};
u64 ef_otiba01_01_modelT[2] = {0};
u64 ef_otiba01_02_modelT[2] = {0};

/* Matrix utility functions for weather particle transforms */
void suMtxMakeSRT(void* mtx, f32 sx, f32 sy, f32 sz, s16 rx, s16 ry, s16 rz, f32 tx, f32 ty, f32 tz) {
    (void)mtx; (void)sx; (void)sy; (void)sz; (void)rx; (void)ry; (void)rz; (void)tx; (void)ty; (void)tz;
}
void suMtxMakeTS(void* mtx, f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz) {
    (void)mtx; (void)tx; (void)ty; (void)tz; (void)sx; (void)sy; (void)sz;
}

/* ============================================================================
 * Name table stubs (m_name_table.c dependencies)
 * ============================================================================ */

/* Player money power calculation */
int mPr_GetMoneyPower(void* player) { (void)player; return 100; }

/* Room/Furniture type conversion */
int mRmTp_FtrItemNo2FtrIdx(int item_no) { (void)item_no; return 0; }

/* ============================================================================
 * MyHouse Actor stubs (ac_my_house.c dependencies)
 * ============================================================================ */

/* House-player mapping */
int mHS_get_pl_no(int house_idx) { (void)house_idx; return 0; }
int mHS_get_pl_no_detail(int house_idx) { (void)house_idx; return 0; }

/* Player achievement checks */
int mPr_CheckFishCompleteTalk(int player_no) { (void)player_no; return 0; }
int mPr_CheckInsectCompleteTalk(int player_no) { (void)player_no; return 0; }

/* Animation frame control */
int cKF_FrameControl_passCheck_now(void* frameCtrl, f32 frame) { (void)frameCtrl; (void)frame; return 0; }

/* Player library functions */
int mPlib_check_label_player_demo_wait(void) { return 0; }
void mPlib_request_main_demo_wait_type1(void* play) { (void)play; }
int mPlib_check_player_outdoor_start(void* game) { (void)game; return 0; }
void mPlib_Set_able_hand_all_item_in_demo(int flag) { (void)flag; }

/* Collision background functions */
void mCoBG_SetPluss5PointOffset_file(void* pos, s16 mode, int data, const char* file, int line) {
    (void)pos; (void)mode; (void)data; (void)file; (void)line;
}
void mCoBG_SetPlussOffset(void* pos, s16 mode, int data) { (void)pos; (void)mode; (void)data; }

/* Math interpolation */
s16 chase_s(s16* value, s16 target, s16 step) { (void)value; (void)target; (void)step; return 0; }

/* Needlework palette lookup */
u16* mNW_PaletteIdx2Palette(int idx) { (void)idx; return NULL; }

/* MyHouse shadow vertices and display lists (8 shadow sets for 4 house levels x 2 orientations) */
Vtx obj_myhome1_shadowE_v[32] = {0};
u64 obj_myhome1_shadowET_model[2] = {0};
Vtx obj_myhome1_shadowW_v[32] = {0};
u64 obj_myhome1_shadowWT_model[2] = {0};

Vtx obj_myhome2_shadowE_v[32] = {0};
u64 obj_myhome2_shadowET_model[2] = {0};
Vtx obj_myhome2_shadowW_v[32] = {0};
u64 obj_myhome2_shadowWT_model[2] = {0};

Vtx obj_myhome3_shadowE_v[32] = {0};
u64 obj_myhome3_shadowET_model[2] = {0};
Vtx obj_myhome3_shadowW_v[32] = {0};
u64 obj_myhome3_shadowWT_model[2] = {0};

Vtx obj_myhome4_shadowE_v[32] = {0};
u64 obj_myhome4_shadowET_model[2] = {0};
Vtx obj_myhome4_shadowW_v[32] = {0};
u64 obj_myhome4_shadowWT_model[2] = {0};

/* MyHouse window display lists (8 sets for 4 house levels x 2 seasons) */
u64 obj_s_myhome1_window_model[2] = {0};
u64 obj_w_myhome1_window_model[2] = {0};
u64 obj_s_myhome2_window_model[2] = {0};
u64 obj_w_myhome2_window_model[2] = {0};
u64 obj_s_myhome3_window_model[2] = {0};
u64 obj_w_myhome3_window_model[2] = {0};
u64 obj_s_myhome4_window_model[2] = {0};
u64 obj_w_myhome4_window_model[2] = {0};

/* MyHouse door mark texture and palette */
u8 obj_myhome_mark_tex_txt[256] = {0};
u16 obj_myhome_mark_pal[16] = {0};

/* Skeleton data structures - minimal stubs for cKF_Skeleton_R_c */
typedef struct {
    int num_shown_joints;
    int num_total_joints;
    void* joint_table;
    void* base_skeleton;
} cKF_Skeleton_R_c_stub;

/* Skeleton base structures (8 for 4 levels x 2 seasons) */
static cKF_Skeleton_R_c_stub g_myhome1_s_skeleton = { 16, 16, NULL, NULL };
static cKF_Skeleton_R_c_stub g_myhome1_w_skeleton = { 16, 16, NULL, NULL };
static cKF_Skeleton_R_c_stub g_myhome2_s_skeleton = { 16, 16, NULL, NULL };
static cKF_Skeleton_R_c_stub g_myhome2_w_skeleton = { 16, 16, NULL, NULL };
static cKF_Skeleton_R_c_stub g_myhome3_s_skeleton = { 16, 16, NULL, NULL };
static cKF_Skeleton_R_c_stub g_myhome3_w_skeleton = { 16, 16, NULL, NULL };
static cKF_Skeleton_R_c_stub g_myhome4_s_skeleton = { 16, 16, NULL, NULL };
static cKF_Skeleton_R_c_stub g_myhome4_w_skeleton = { 16, 16, NULL, NULL };

cKF_Skeleton_R_c_stub* cKF_bs_r_obj_s_myhome1 = &g_myhome1_s_skeleton;
cKF_Skeleton_R_c_stub* cKF_bs_r_obj_w_myhome1 = &g_myhome1_w_skeleton;
cKF_Skeleton_R_c_stub* cKF_bs_r_obj_s_myhome2 = &g_myhome2_s_skeleton;
cKF_Skeleton_R_c_stub* cKF_bs_r_obj_w_myhome2 = &g_myhome2_w_skeleton;
cKF_Skeleton_R_c_stub* cKF_bs_r_obj_s_myhome3 = &g_myhome3_s_skeleton;
cKF_Skeleton_R_c_stub* cKF_bs_r_obj_w_myhome3 = &g_myhome3_w_skeleton;
cKF_Skeleton_R_c_stub* cKF_bs_r_obj_s_myhome4 = &g_myhome4_s_skeleton;
cKF_Skeleton_R_c_stub* cKF_bs_r_obj_w_myhome4 = &g_myhome4_w_skeleton;

/* Animation data structures (16 sets for door animations) */
typedef struct { int frame_count; void* data; } cKF_Animation_R_c_stub;
static cKF_Animation_R_c_stub g_myhome_anim_stub = { 0, NULL };

cKF_Animation_R_c_stub* cKF_ba_r_obj_s_myhome1 = &g_myhome_anim_stub;
cKF_Animation_R_c_stub* cKF_ba_r_obj_w_myhome1 = &g_myhome_anim_stub;
cKF_Animation_R_c_stub* cKF_ba_r_obj_s_myhome2 = &g_myhome_anim_stub;
cKF_Animation_R_c_stub* cKF_ba_r_obj_w_myhome2 = &g_myhome_anim_stub;
cKF_Animation_R_c_stub* cKF_ba_r_obj_s_myhome3 = &g_myhome_anim_stub;
cKF_Animation_R_c_stub* cKF_ba_r_obj_w_myhome3 = &g_myhome_anim_stub;
cKF_Animation_R_c_stub* cKF_ba_r_obj_s_myhome4 = &g_myhome_anim_stub;
cKF_Animation_R_c_stub* cKF_ba_r_obj_w_myhome4 = &g_myhome_anim_stub;

cKF_Animation_R_c_stub* cKF_ba_r_obj_s_myhome1_out = &g_myhome_anim_stub;
cKF_Animation_R_c_stub* cKF_ba_r_obj_w_myhome1_out = &g_myhome_anim_stub;
cKF_Animation_R_c_stub* cKF_ba_r_obj_s_myhome2_out = &g_myhome_anim_stub;
cKF_Animation_R_c_stub* cKF_ba_r_obj_w_myhome2_out = &g_myhome_anim_stub;
cKF_Animation_R_c_stub* cKF_ba_r_obj_s_myhome3_out = &g_myhome_anim_stub;
cKF_Animation_R_c_stub* cKF_ba_r_obj_w_myhome3_out = &g_myhome_anim_stub;
cKF_Animation_R_c_stub* cKF_ba_r_obj_s_myhome4_out = &g_myhome_anim_stub;
cKF_Animation_R_c_stub* cKF_ba_r_obj_w_myhome4_out = &g_myhome_anim_stub;

/* ============================================================================
 * Dump Actor stubs (ac_dump.c dependencies)
 * ============================================================================ */

/* Dump shadow vertices and display list - requires binary model loader */
Vtx obj_dump_shadow_v[64] = {0};
u64 obj_dump_shadowT_model[2] = {0};

/* Dump display lists (summer and winter) - requires binary model loader */
u64 dump_s_DL_model[2] = {0};
u64 dump_w_DL_model[2] = {0};
