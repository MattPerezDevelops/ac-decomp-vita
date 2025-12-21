/**
 * @file gc_stubs.c
 * @brief Stubs for GameCube-specific systems
 *
 * This file provides stub implementations for GameCube SDK and
 * JSystem functions that are not needed or replaced on PC.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "pc/platform.h"
#include "game.h"  /* For GAME type */
#include "gamealloc.h"  /* For GameAlloc type */
#include "m_card.h"  /* For card functions */
#include "Famicom/famicom.h"  /* For famicom types */
#include "libforest/gbi_extensions.h"  /* For gsSPNTrianglesInit_5b and other Dolphin GBI */

/* ============================================================================
 * JSystem Wrapper Stubs (jsyswrap.h)
 * NOTE: JW_BeginFrame and JW_EndFrame are now implemented in main.c
 * ============================================================================ */

/* JW_Init2 and JW_Init3 are defined later in this file after AramArchiveHandle */

/* JKRAram - Audio RAM management (not needed on PC) */
void* JC_JKRAram_getAramHeap(void) {
    return NULL;
}

void JC_JKRAramHeap_dump(void* heap) {
    (void)heap;
}

/* ============================================================================
 * Dolphin OS Stubs
 * ============================================================================ */

/* These may already be in os_compat.c - only add if not present */

void OSReport(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

/* ============================================================================
 * Video Interface Stubs (dolphin/vi.h)
 * ============================================================================ */

void VISetBlack(int black) {
    (void)black;
    /* TODO: Could set window to black if needed */
}

void VIFlush(void) {
    /* No-op */
}

void VIWaitForRetrace(void) {
    /* No-op - vsync handled by SDL */
}

/* ============================================================================
 * DVD Stubs - Now in dvd_compat.c
 * ============================================================================ */
/* DVDCheckDisk and other DVD functions are now in dvd_compat.c */

/* ============================================================================
 * IRQ Manager Stubs (irqmgr.h)
 * ============================================================================ */

/* IRQ reset status */
int ResetStatus = 0;

typedef struct {
    void* mq;
} irqmgr_client_t;

void CreateIRQManager(void* stack, int stack_size, int pri, int arg) {
    (void)stack; (void)stack_size; (void)pri; (void)arg;
}

void irqmgr_AddClient(void* client, void* mq) {
    (void)client; (void)mq;
}

/* ============================================================================
 * Pad Manager Stubs (padmgr.h)
 * These wrap to our input_compat.c functions
 * ============================================================================ */

void padmgr_Init(void* arg) {
    (void)arg;
}

void padmgr_Create(void* mq, int a, int b, void* stack, int stack_size) {
    (void)mq; (void)a; (void)b; (void)stack; (void)stack_size;
}

int padmgr_isConnectedController(int port) {
    /* Always report controller connected for port 0 */
    return (port == 0) ? 1 : 0;
}

/* ============================================================================
 * Memory Card Stubs (m_card.h)
 * TODO: Replace with file-based saves
 * ============================================================================ */

void mCD_init_card(void) {
    printf("PC: Memory card init (using file saves)\n");
}

void mCD_save_data_aram_malloc(void) {
    /* No-op */
}

int mCD_GetThisLandSlotNo_code(int* player_no, s32* slot_card_results) {
    (void)player_no; (void)slot_card_results;
    return 0;
}

/* ============================================================================
 * Boot Stubs (boot.h)
 * ============================================================================ */

void* HotStartEntry = NULL;

void osShutdownStart(int type) {
    /* osShutdown is defined in os_compat.c */
    extern int osShutdown;
    printf("PC: Shutdown requested (type=%d)\n", type);
    osShutdown = type;
    if (type == 0) {  /* OS_RESET_SHUTDOWN */
        exit(0);
    }
}

void HotResetIplMenu(void) {
    printf("PC: Reset to IPL menu requested\n");
    exit(0);
}

int APPNMI_HOTRESET_GET(void) {
    return 0;
}

/* ============================================================================
 * Debug Stubs (m_debug.h)
 * ============================================================================ */

void new_Debug_mode(void) {
    /* No-op */
}

/* Debug registers */
static s32 debug_regs[256][256];

s32 GETREG(int type, int idx) {
    if (type >= 0 && type < 256 && idx >= 0 && idx < 256) {
        return debug_regs[type][idx];
    }
    return 0;
}

void SETREG(int type, int idx, s32 val) {
    if (type >= 0 && type < 256 && idx >= 0 && idx < 256) {
        debug_regs[type][idx] = val;
    }
}

/* ============================================================================
 * Zurumode Stubs (debug mode)
 * ============================================================================ */

int zurumode_flag = 0;

void zurumode_init(void) {
    zurumode_flag = 0;
}

void zurumode_cleanup(void) {
    /* No-op */
}

/* ============================================================================
 * BGM / Audio Stubs (m_bgm.h, audio.h)
 * TODO: Connect to jaudio_NES when porting audio
 * ============================================================================ */

/* mBGM_ct is now in m_bgm.c */

void Na_RestartPrepare(void) {
    /* No-op */
}

int Na_CheckRestartReady(void) {
    return 1;  /* Always ready */
}

void Na_Restart(void) {
    /* No-op */
}

/* ============================================================================
 * Vibration Controller Stubs (m_vibctl.h)
 * ============================================================================ */

void mVibctl_ct(void) {
    /* No-op */
}

/* Common data stubs moved to end of file where common_data_t is available */

/* ============================================================================
 * Famicom Emulator Stubs (Famicom/famicom.h)
 * The NES emulator is optional for the PC port
 * ============================================================================ */

void famicom_mount_archive(void) {
    /* No-op - NES games not loaded */
}

void famicom_setCallback_getSaveChan(FAMICOM_GETSAVECHAN_PROC proc) {
    (void)proc;
}

/* ============================================================================
 * Message System Stubs (m_msg.h)
 * ============================================================================ */

void mMsg_aram_init2(void) {
    /* No-op */
}

/* ============================================================================
 * Land Data Stubs (m_land.h)
 * ============================================================================ */

void mLd_StartFlagOn(void) {
    /* No-op */
}

/* ============================================================================
 * ROM Check Stubs (sys_romcheck.h)
 * ============================================================================ */

void sys_romcheck(void) {
    /* No-op - not needed on PC */
}

/* ============================================================================
 * System Math (sys_math.h)
 * ============================================================================ */

static u32 rnd_seed = 12345;

void init_rnd(void) {
    rnd_seed = 12345;
}

u32 get_rnd(void) {
    rnd_seed = rnd_seed * 1103515245 + 12345;
    return rnd_seed;
}

/* ============================================================================
 * Microcode System (sys_ucode.h)
 * ============================================================================ */

void* ucode_GetPolyTextStart(void) {
    return NULL;  /* Not used on PC */
}

void* ucode_GetSpriteTextStart(void) {
    return NULL;  /* Not used on PC */
}

/* ============================================================================
 * libultra OS Initialize - now in os_compat.c
 * ============================================================================ */

/* NOTE: __osInitialize_common is implemented in os_compat.c */

/* ============================================================================
 * Graph.c Dependencies - Stubs for game systems
 * These will be replaced with real implementations later
 * ============================================================================ */

#include "pc/gbi.h"  /* For Gfx type and GBI macros */
#include "sys_dynamic.h"

/* Global display list memory */
dynamic_t sys_dynamic;

/* Debug mode - needs to be a real struct with register storage */
#include "m_debug.h"
static Debug_mode debug_mode_storage;
Debug_mode* debug_mode = &debug_mode_storage;

/* Game state transition table is now in m_game_dlftbls.c */
/* NOTE: game_dlftbls[] is now defined in src/game/m_game_dlftbls.c */

/* Game init/cleanup functions for each game state */
/* first_game_init/cleanup is provided by first_game.c */
void select_init(GAME* game) { (void)game; }
void select_cleanup(GAME* game) { (void)game; }
/* play_init/cleanup is provided by m_play.c */
/* second_game_init/cleanup is provided by second_game.c */
/* trademark_init/cleanup is provided by m_trademark.c */
void player_select_init(GAME* game) { (void)game; }
void player_select_cleanup(GAME* game) { (void)game; }
void save_menu_init(GAME* game) { (void)game; }
void save_menu_cleanup(GAME* game) { (void)game; }
void famicom_emu_init(GAME* game) { (void)game; }
void famicom_emu_cleanup(GAME* game) { (void)game; }
void prenmi_init(GAME* game) { (void)game; }
void prenmi_cleanup(GAME* game) { (void)game; }
/* Note: game_get_next_game_init is provided by game.c */

/* More missing symbols from graph.c */
u64 gspF3DZEX2_NoN_PosLight_fifoDataStart[1];
u64 gspF3DZEX2_NoN_PosLight_fifoTextStart[1];
u64 gspS2DEX2_fifoDataStart[1];
u64 gspS2DEX2_fifoTextStart[1];

u64* ucode_GetPolyDataStart(void) {
    return gspF3DZEX2_NoN_PosLight_fifoDataStart;
}

u64* ucode_GetSpriteDataStart(void) {
    return gspS2DEX2_fifoDataStart;
}

/* ============================================================================
 * More Graph.c Dependencies
 * ============================================================================ */

/* Debug hungup flag */
void _dbg_hungup(const char* file, int line) {
    printf("PC: Debug hungup at %s:%d\n", file, line);
}

/* BGM and audio reset - now in m_bgm.c and audio.c */
/* mBGM_reset is now in m_bgm.c */
void mVibctl_reset(void) { }
/* sAdo_SoftReset is now in audio.c */
/* sAdo_GameFrame is now in audio.c */

/* Reset time tracking */
int ResetTime = 0;

/* Game controller, state machine functions now provided by game.c:
 * - game_get_controller
 * - game_class_p
 * - game_main
 * - game_ct
 * - game_dt
 * - game_is_doing
 */

/* DVD error draw (not needed on PC) - returns 0 to indicate no error */
int dvderr_draw(void) {
    return 0;
}

/* ============================================================================
 * Game.c Dependencies
 * ============================================================================ */

/* Debug mode functions */
void Debug_mode_input(void* pad) { (void)pad; }
void Debug_mode_output(void* graph) { (void)graph; }

/* JSystem process bar (debug UI) */
void* JC_JUTProcBar_getManager(void) { return NULL; }
void JC_JUTProcBar_setVisible(void* mgr, int visible) { (void)mgr; (void)visible; }
void JC_JUTProcBar_setVisibleHeapBar(void* mgr, int visible) { (void)mgr; (void)visible; }

/* JSystem display manager */
void* JC_JFWDisplay_getManager(void) { return NULL; }
void JC_JFWDisplay_setFrameRate(void* mgr, int rate) { (void)mgr; (void)rate; }

/* Gfx allocation */
Gfx* gfxopen(Gfx* disp) {
    /* Return the passed display list pointer - actual implementation would track */
    return disp;
}

Gfx* gfxclose(Gfx* start, Gfx* end) {
    (void)start;
    return end;
}

/* Padmgr functions */
void padmgr_RequestPadData(void* mq, int timeout) { (void)mq; (void)timeout; }
void padmgr_ClearPadData(void) { }

/* Time manager */
void mTM_time(void* game) { (void)game; }

/* BGM functions - now in m_bgm.c */
/* mBGM_main, mBGM_init, mBGM_cleanup are now in m_bgm.c */

/* Game allocator */
void* gamealloc_malloc(GameAlloc* gamealloc, size_t size) {
    (void)gamealloc;
    return malloc(size);
}

void gamealloc_free(GameAlloc* gamealloc, void* ptr) {
    (void)gamealloc;
    free(ptr);
}

void gamealloc_init(GameAlloc* gamealloc) {
    (void)gamealloc;
}

/* Arena free space */
size_t GetFreeArena(void) {
    return 1024 * 1024 * 16;  /* Report 16MB free */
}

/* Controller */
void mCon_ct(void* game) { (void)game; }
void mCon_dt(void* game) { (void)game; }

/* GFX print default flags */
u8 __gfxprint_default_flags = 0;

/* GBA Link (not needed on PC) */
void GBAInit(void) { }

/* Vibration init/cleanup */
void mVibctl_init(void) { }
void mVibctl_cleanup(void) { }

/* Game allocator cleanup */
void gamealloc_cleanup(GameAlloc* gamealloc) { (void)gamealloc; }

/* DVD DiskID - normally located at 0x80000000 on GameCube */
#include "dolphin/dvd.h"
DVDDiskID DiskID = {
    .gameName = "GAFE",   /* Game code */
    .company = "01",       /* Company code */
    .diskNumber = 0,
    .gameVersion = 0,
    .streaming = 0,
    .streamBufSize = 0
};

/* ============================================================================
 * Common Data (m_common_data.h)
 * This is the main game state structure - zero-initialized for now
 * ============================================================================ */
#include "m_common_data.h"
common_data_t common_data;

void common_data_init(void) {
    memset(&common_data, 0, sizeof(common_data));
}

void common_data_reinit(void) {
    /* No-op for now */
}

void common_data_clear(void) {
    memset(&common_data, 0, sizeof(common_data));
}

/* ============================================================================
 * Trademark/Title Screen Dependencies
 * These need includes already pulled in by m_common_data.h above
 * ============================================================================ */
#include "m_npc.h"
#include "m_view.h"

/* NPC system */
void mNpc_SetAnimalTitleDemo(mNpc_demo_npc_c* demo_npc, Animal_c* animal, GAME* game) {
    (void)demo_npc; (void)animal; (void)game;
}
void mNpc_SetNpcList(mNpc_NpcList_c* npclist, Animal_c* animal, int count, int malloc_flag) {
    (void)npclist; (void)animal; (void)count; (void)malloc_flag;
}
void mNpc_ClearCacheName(void) { }
void mNpc_ClearInAnimal(void) { }
void mNpc_FirstClearGoodbyMail(void) { }
void mNpc_ClearIslandNpcRoomData(void) { }

/* Controller pak */
void mCPk_InitPak(void) { }

/* Flashrom/Save system */
int mFRm_CheckSaveData(void* save) { (void)save; return 0; }
void mFRm_ClearSaveCheckData(mFRm_chk_t* save_check) { (void)save_check; }
void mFRm_clear_err_info(void) { }

/* Private data */
void mPr_ClearPrivateInfo(Private_c* private_info) { (void)private_info; }
void mPr_RandomSetPlayerData_title_demo(Private_c* priv) { (void)priv; }

/* Events - now in m_event.c */
/* mEv_ClearEventInfo, mEv_CheckTitleDemo, mEv_SetTitleDemo are now in m_event.c */

/* Time */
void mTM_set_season(void* time) { (void)time; }

/* Title demo - now in m_titledemo.c */
/* mTD_get_titledemo_no, mTD_demono_get are now in m_titledemo.c */

/* Display list functions - now in m_rcp.c */
/* DisplayList_initialize, fade_black_draw are now in m_rcp.c */

/* =============================================================================
 * Nintendo Logo - With Texture Support
 *
 * For the PC build, we generate the logo display list at runtime since
 * static Gfx initializers with pointer casts aren't constant expressions in C.
 *
 * This implements the same logic as logo_ninT_model from logo_nin.c:
 * - Set texture image (G_SETTIMG_DOLPHIN)
 * - Configure tile (G_SETTILE_DOLPHIN)
 * - Load 4 vertices forming a quad with texture coords
 * - Draw 2 triangles using G_TRIN_INDEPEND
 * ============================================================================= */

/* Include the actual Nintendo logo texture data */
#include "assets/bootdata/nintendo_376x104.inc"

/* Logo vertex data with texture coordinates
 * Format: {ob[0], ob[1], ob[2], flag, tc[0], tc[1], cn[0], cn[1], cn[2], cn[3]} */
static Vtx logo_nin_v[4];

/* Initialize the logo vertex data (called once) */
static void init_logo_vertices(void) {
    static int initialized = 0;
    if (initialized) return;
    initialized = 1;

    /* Vertex 0: top-left */
    logo_nin_v[0].v.ob[0] = -187;
    logo_nin_v[0].v.ob[1] = 70;
    logo_nin_v[0].v.ob[2] = 0;
    logo_nin_v[0].v.flag = 1;
    logo_nin_v[0].v.tc[0] = 0;
    logo_nin_v[0].v.tc[1] = 0;
    logo_nin_v[0].v.cn[0] = 255;  /* White to show texture */
    logo_nin_v[0].v.cn[1] = 255;
    logo_nin_v[0].v.cn[2] = 255;
    logo_nin_v[0].v.cn[3] = 255;

    /* Vertex 1: bottom-left */
    logo_nin_v[1].v.ob[0] = -187;
    logo_nin_v[1].v.ob[1] = -34;
    logo_nin_v[1].v.ob[2] = 0;
    logo_nin_v[1].v.flag = 1;
    logo_nin_v[1].v.tc[0] = 0;
    logo_nin_v[1].v.tc[1] = 3328;  /* 104 * 32 */
    logo_nin_v[1].v.cn[0] = 255;
    logo_nin_v[1].v.cn[1] = 255;
    logo_nin_v[1].v.cn[2] = 255;
    logo_nin_v[1].v.cn[3] = 255;

    /* Vertex 2: top-right */
    logo_nin_v[2].v.ob[0] = 189;
    logo_nin_v[2].v.ob[1] = 70;
    logo_nin_v[2].v.ob[2] = 0;
    logo_nin_v[2].v.flag = 1;
    logo_nin_v[2].v.tc[0] = 12032;  /* 376 * 32 */
    logo_nin_v[2].v.tc[1] = 0;
    logo_nin_v[2].v.cn[0] = 255;
    logo_nin_v[2].v.cn[1] = 255;
    logo_nin_v[2].v.cn[2] = 255;
    logo_nin_v[2].v.cn[3] = 255;

    /* Vertex 3: bottom-right */
    logo_nin_v[3].v.ob[0] = 189;
    logo_nin_v[3].v.ob[1] = -34;
    logo_nin_v[3].v.ob[2] = 0;
    logo_nin_v[3].v.flag = 1;
    logo_nin_v[3].v.tc[0] = 12032;  /* 376 * 32 */
    logo_nin_v[3].v.tc[1] = 3328;   /* 104 * 32 */
    logo_nin_v[3].v.cn[0] = 255;
    logo_nin_v[3].v.cn[1] = 255;
    logo_nin_v[3].v.cn[2] = 255;
    logo_nin_v[3].v.cn[3] = 255;

    printf("[LOGO] Logo vertices initialized at %p, texture at %p\n",
           (void*)logo_nin_v, (void*)nintendo_376x104);
    fflush(stdout);
}

void make_dl_nintendo_logo(Gfx** gpp, u32 alpha) {
    Gfx* g = *gpp;

    /* Initialize vertices on first call */
    init_logo_vertices();

    printf("[LOGO] make_dl_nintendo_logo: gpp=%p, *gpp=%p, alpha=%u\n",
           (void*)gpp, (void*)g, alpha);
    fflush(stdout);

    /* Set primitive color with alpha for fade effect - use Nintendo red */
    gDPSetPrimColor(g++, 0, 255, 220, 0, 0, alpha);

    /* Set cycle type to 1-cycle for textured rendering */
    gDPSetCycleType(g++, G_CYC_1CYCLE);

    /* Set up texture - G_SETTIMG_DOLPHIN format:
     * w0 = (0xFD << 24) | (fmt << 21) | (siz << 19) | (1 << 18) | ((h/4-1) << 10) | (w-1)
     * w1 = texture address
     * fmt = 4 (G_IM_FMT_I), siz = 0 (4-bit), w = 376, h = 104
     */
    {
        u32 fmt = 4;   /* G_IM_FMT_I */
        u32 siz = 0;   /* G_IM_SIZ_4b */
        u32 w = 376;
        u32 h = 104;
        g->words.w0 = (0xFD << 24) | (fmt << 21) | (siz << 19) | (1 << 18) |
                      (((h/4)-1) << 10) | (w - 1);
        g->words.w1 = (u32)(uintptr_t)nintendo_376x104;
        g++;
        printf("[LOGO] G_SETTIMG_DOLPHIN: fmt=%d siz=%d w=%d h=%d addr=%p\n",
               fmt, siz, w, h, (void*)nintendo_376x104);
    }

    /* Set up tile - G_SETTILE_DOLPHIN (0xD2) format:
     * w0 = (0xD2 << 24) | (d_fmt << 20) | (tile << 16) | (tlut << 12) |
     *      (wrap_s << 10) | (wrap_t << 8) | (shift_s << 4) | shift_t
     * w1 = 0
     */
    {
        u32 d_fmt = 15;  /* G_DOLPHIN_TLUT_DEFAULT_MODE */
        u32 tile = 0;
        u32 tlut = 15;
        u32 wrap_s = 0;  /* GX_CLAMP */
        u32 wrap_t = 0;  /* GX_CLAMP */
        g->words.w0 = (0xD2 << 24) | (d_fmt << 20) | (tile << 16) | (tlut << 12) |
                      (wrap_s << 10) | (wrap_t << 8);
        g->words.w1 = 0;
        g++;
        printf("[LOGO] G_SETTILE_DOLPHIN: d_fmt=%d tile=%d tlut=%d\n", d_fmt, tile, tlut);
    }

    /* Enable texture */
    gSPTexture(g++, 0xFFFF, 0xFFFF, 0, 0, G_ON);

    /* Load 4 vertices starting at index 0 */
    gSPVertex(g++, &logo_nin_v[0], 4, 0);

    /* Draw 2 triangles using G_TRIN_INDEPEND (0x0A) command */
    gSPNTrianglesInit_5b(g++,
        2,           /* n = 2 triangles */
        0, 1, 2,     /* tri0: top-left, bottom-left, top-right */
        1, 3, 2,     /* tri1: bottom-left, bottom-right, top-right */
        0, 0, 0      /* tri2: unused */
    );

    printf("[LOGO] Logo display list generated: %ld commands\n", (long)(g - *gpp));
    fflush(stdout);

    *gpp = g;
}

/* BGM/Audio - now in m_bgm.c */
/* mBGMPsComp_make_ps_lost_fanfare, mBGMPsComp_scene_mode are now in m_bgm.c */

/* Random */
f32 fqrand(void) { return (f32)rand() / RAND_MAX; }

/* Mail */
void mMl_clear_mail_box(Mail_c* mail, int num) { (void)mail; (void)num; }

/* JSystem wrapper */
void JW_SetLogoMode(int mode) { (void)mode; }

/* Field info */
void mFI_SetClimate(int climate) { (void)climate; }

/* View/Matrix - initView is now in m_view.c */
/* new_Matrix moved to linker_stubs.c (has full matrix stack implementation) */
void viBlack(void) { }

/* Message system */
void mMsg_aram_init(void) { }

/* Quest */
void mQst_ClearGrabItemInfo(void) { }

/* Island */
void mISL_ClearKeepIsland(void) { }

/* Background item */
void mBI_ct(void) { }

/* Font */
void mFont_ct(void) { }

/* Vibration */
void mVibctl_init0(void) { }

/* Memory card */
void mCD_InitAll(void) { }

/* ============================================================================
 * m_play.c Dependencies - Stubs for missing collision/environment functions
 * ============================================================================ */

/* Collision background system */
void mCoBG_CalcTimerDecalCircle(void) { }
void mCoBG_InitBlockBgCheckMode(void) { }
void mCoBG_InitDecalCircle(void) { }
void mCoBG_InitMoveBgData(void) { }

/* Environment - mEnv_WindMove is now in m_kankyo.c */

/* JSystem wrapper */
void JW_JUTReport(int level, const char* fmt, ...) {
    (void)level;
    (void)fmt;
    /* JUT report - no-op on PC */
}

/* ============================================================================
 * GameCube cache functions - no-op on PC
 * ============================================================================ */

void DCStoreRangeNoSync(void* addr, u32 size) {
    (void)addr;
    (void)size;
    /* Data cache store - not needed on PC */
}

void DCFlushRange(void* addr, u32 size) {
    (void)addr;
    (void)size;
    /* Data cache flush - not needed on PC */
}

void DCFlushRangeNoSync(void* addr, u32 size) {
    (void)addr;
    (void)size;
    /* Data cache flush no sync - not needed on PC */
}

/* ============================================================================
 * libultra gu (graphics utility) functions
 * ============================================================================ */

#include "libultra/gu.h"
#include <math.h>

void guMtxIdentF(float mf[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            mf[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

void guMtxF2L(float mf[4][4], Mtx *m) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            s32 fixed = (s32)(mf[i][j] * 65536.0f);
            m->fp.intPart[i][j] = (u16)(fixed >> 16);
            m->fp.fracPart[i][j] = (u16)(fixed & 0xFFFF);
        }
    }
}

void guTranslate(Mtx *m, float x, float y, float z) {
    float mf[4][4];
    guTranslateF(mf, x, y, z);
    guMtxF2L(mf, m);
}

void guScale(Mtx *m, float x, float y, float z) {
    float mf[4][4];
    guScaleF(mf, x, y, z);
    guMtxF2L(mf, m);
}

void guMtxIdent(Mtx *m) {
    float mf[4][4];
    guMtxIdentF(mf);
    guMtxF2L(mf, m);
}

void guNormalize(float *x, float *y, float *z) {
    float len = sqrtf((*x)*(*x) + (*y)*(*y) + (*z)*(*z));
    if (len > 0.0f) {
        *x /= len;
        *y /= len;
        *z /= len;
    }
}

void guRotate(Mtx* m, float a, float x, float y, float z) {
    float mf[4][4];
    float rad = a * 3.14159265f / 180.0f;
    float c = cosf(rad);
    float s = sinf(rad);
    float one_minus_c = 1.0f - c;

    guNormalize(&x, &y, &z);

    guMtxIdentF(mf);
    mf[0][0] = x*x*one_minus_c + c;
    mf[0][1] = x*y*one_minus_c - z*s;
    mf[0][2] = x*z*one_minus_c + y*s;
    mf[1][0] = y*x*one_minus_c + z*s;
    mf[1][1] = y*y*one_minus_c + c;
    mf[1][2] = y*z*one_minus_c - x*s;
    mf[2][0] = z*x*one_minus_c - y*s;
    mf[2][1] = z*y*one_minus_c + x*s;
    mf[2][2] = z*z*one_minus_c + c;

    guMtxF2L(mf, m);
}

void guOrtho(Mtx *m, float l, float r, float b, float t, float n, float f, float scale) {
    float mf[4][4];
    guMtxIdentF(mf);

    mf[0][0] = 2.0f / (r - l);
    mf[1][1] = 2.0f / (t - b);
    mf[2][2] = -2.0f / (f - n);
    mf[3][0] = -(r + l) / (r - l);
    mf[3][1] = -(t + b) / (t - b);
    mf[3][2] = -(f + n) / (f - n);

    /* Apply scale */
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            mf[i][j] *= scale;
        }
    }

    guMtxF2L(mf, m);
}

void guPerspective(Mtx *m, u16 *perspNorm, float fovy, float aspect,
                   float near, float far, float scale) {
    float mf[4][4];
    float cot = 1.0f / tanf(fovy * 3.14159265f / 360.0f);

    guMtxIdentF(mf);
    mf[0][0] = cot / aspect;
    mf[1][1] = cot;
    mf[2][2] = (near + far) / (near - far);
    mf[2][3] = -1.0f;
    mf[3][2] = 2.0f * near * far / (near - far);
    mf[3][3] = 0.0f;

    /* Apply scale */
    for (int i = 0; i < 4; i++) {
        mf[i][0] *= scale;
        mf[i][1] *= scale;
    }

    guMtxF2L(mf, m);

    /* Perspective normalization value */
    if (perspNorm) {
        *perspNorm = (u16)(65536.0f * near / (near - far));
    }
}

void guLookAt(Mtx *m,
              float xEye, float yEye, float zEye,
              float xAt, float yAt, float zAt,
              float xUp, float yUp, float zUp) {
    float mf[4][4];
    float fx, fy, fz;
    float ux, uy, uz;
    float sx, sy, sz;

    /* Forward vector (looking direction) */
    fx = xAt - xEye;
    fy = yAt - yEye;
    fz = zAt - zEye;
    guNormalize(&fx, &fy, &fz);

    /* Side vector (cross product of forward and up) */
    sx = fy * zUp - fz * yUp;
    sy = fz * xUp - fx * zUp;
    sz = fx * yUp - fy * xUp;
    guNormalize(&sx, &sy, &sz);

    /* Recompute up (cross product of side and forward) */
    ux = sy * fz - sz * fy;
    uy = sz * fx - sx * fz;
    uz = sx * fy - sy * fx;

    guMtxIdentF(mf);
    mf[0][0] = sx;  mf[1][0] = sy;  mf[2][0] = sz;
    mf[0][1] = ux;  mf[1][1] = uy;  mf[2][1] = uz;
    mf[0][2] = -fx; mf[1][2] = -fy; mf[2][2] = -fz;
    mf[3][0] = -(sx * xEye + sy * yEye + sz * zEye);
    mf[3][1] = -(ux * xEye + uy * yEye + uz * zEye);
    mf[3][2] = (fx * xEye + fy * yEye + fz * zEye);

    guMtxF2L(mf, m);
}

void guLookAtHilite(Mtx *m, LookAt *l, Hilite *h,
                    float xEye, float yEye, float zEye,
                    float xAt, float yAt, float zAt,
                    float xUp, float yUp, float zUp,
                    float xl1, float yl1, float zl1,
                    float xl2, float yl2, float zl2,
                    int twidth, int theight) {
    (void)l; (void)h;
    (void)xl1; (void)yl1; (void)zl1;
    (void)xl2; (void)yl2; (void)zl2;
    (void)twidth; (void)theight;

    /* For now just do a basic lookAt */
    guLookAt(m, xEye, yEye, zEye, xAt, yAt, zAt, xUp, yUp, zUp);

    /* TODO: Properly compute hilite texgen values */
    if (l) {
        memset(l, 0, sizeof(LookAt));
    }
    if (h) {
        memset(h, 0, sizeof(Hilite));
    }
}

signed short sins(unsigned short angle) {
    return (signed short)(sinf((float)angle * 3.14159265f / 32768.0f) * 32767.0f);
}

signed short coss(unsigned short angle) {
    return (signed short)(cosf((float)angle * 3.14159265f / 32768.0f) * 32767.0f);
}

/* ============================================================================
 * fbdemo_wipe1 stubs (asset file not available)
 * ============================================================================ */

#include "m_fbdemo_wipe1.h"

void fbdemo_wipe1_startup(fbdemo_wipe1* this) { (void)this; }
fbdemo_wipe1* fbdemo_wipe1_init(fbdemo_wipe1* this) { return this; }
void fbdemo_wipe1_move(fbdemo_wipe1* this, int update_rate) { (void)this; (void)update_rate; }
void fbdemo_wipe1_draw(fbdemo_wipe1* this, Gfx** gfx) { (void)this; (void)gfx; }
u8 fbdemo_wipe1_is_finish(fbdemo_wipe1* this) { (void)this; return 1; /* Always finished */ }
void fbdemo_wipe1_settype(fbdemo_wipe1* this, int type) { (void)this; (void)type; }
void fbdemo_wipe1_setcolor_rgba8888(fbdemo_wipe1* this, u32 color) {
    (void)this; (void)color;
}

/* ============================================================================
 * m_msg stubs (asset files not available)
 * ============================================================================ */

#include "m_msg.h"

static mMsg_Window_c dummy_msg_window;

void mMsg_ct(GAME* game) { (void)game; }
void mMsg_dt(GAME* game) { (void)game; }
void mMsg_Main(GAME* game) { (void)game; }
void mMsg_Draw(GAME* game) { (void)game; }
mMsg_Window_c* mMsg_Get_base_window_p() { return &dummy_msg_window; }
int mMsg_Check_request_priority(mMsg_Window_c* msg_p, int request_priority) {
    (void)msg_p; (void)request_priority; return 0;
}
int mMsg_Check_main_index(mMsg_Window_c* msg_p, int main_index) {
    (void)msg_p; (void)main_index; return 0;
}
int mMsg_Check_main_wait(mMsg_Window_c* msg_p) { (void)msg_p; return 0; }
int mMsg_Check_not_series_main_wait(mMsg_Window_c* msg_p) { (void)msg_p; return 0; }
int mMsg_Check_main_hide(mMsg_Window_c* msg_p) { (void)msg_p; return 1; }
int mMsg_request_main_forceoff() { return 0; }
int mMsg_request_main_disappear(mMsg_Window_c* msg_p) { (void)msg_p; return 0; }
int mMsg_request_main_appear(mMsg_Window_c* msg_p, ACTOR* client_actor_p, int show_name, rgba_t* window_color,
                              int msg_no, int request_priority) {
    (void)msg_p; (void)client_actor_p; (void)show_name; (void)window_color;
    (void)msg_no; (void)request_priority; return 0;
}
int mMsg_request_main_disappear_wait_type1(mMsg_Window_c* msg_p) { (void)msg_p; return 0; }
int mMsg_request_main_disappear_wait_type2(mMsg_Window_c* msg_p) { (void)msg_p; return 0; }
int mMsg_request_main_appear_wait_type2(mMsg_Window_c* msg_p, int clear_flag) { (void)msg_p; (void)clear_flag; return 0; }
int mMsg_request_main_appear_wait_type1(mMsg_Window_c* msg_p) { (void)msg_p; return 0; }
void mMsg_Set_free_str(mMsg_Window_c* msg_p, int free_str_no, u8* str, int str_len) {
    (void)msg_p; (void)free_str_no; (void)str; (void)str_len;
}
void mMsg_Set_free_str_cl(mMsg_Window_c* msg_p, int str_no, u8* str_p, int str_len, int cl_id) {
    (void)msg_p; (void)str_no; (void)str_p; (void)str_len; (void)cl_id;
}
void mMsg_Set_free_str_art(mMsg_Window_c* msg_p, int str_no, u8* str_p, int str_len, int article) {
    (void)msg_p; (void)str_no; (void)str_p; (void)str_len; (void)article;
}
void mMsg_Set_free_str_cl_art(mMsg_Window_c* msg_p, int str_no, u8* str_p, int str_len, int cl_id, int article) {
    (void)msg_p; (void)str_no; (void)str_p; (void)str_len; (void)cl_id; (void)article;
}
void mMsg_Set_item_str(mMsg_Window_c* msg_p, int item_str_no, u8* str, int str_len) {
    (void)msg_p; (void)item_str_no; (void)str; (void)str_len;
}
void mMsg_Set_item_str_art(mMsg_Window_c* msg_p, int item_str_no, u8* str, int str_len, int article) {
    (void)msg_p; (void)item_str_no; (void)str; (void)str_len; (void)article;
}
void mMsg_Set_mail_str(mMsg_Window_c* msg_p, int mail_str_no, u8* str, int str_len) {
    (void)msg_p; (void)mail_str_no; (void)str; (void)str_len;
}
void mMsg_Set_continue_msg_num(mMsg_Window_c* msg_p, int msg_no) { (void)msg_p; (void)msg_no; }
int mMsg_Get_msg_num(mMsg_Window_c* msg_p) { (void)msg_p; return 0; }
int mMsg_ChangeMsgData(mMsg_Window_c* msg_p, int index) { (void)msg_p; (void)index; return 0; }
int mMsg_Check_NowUtter() { return 0; }
int mMsg_Get_Length_String(u8* str, int str_len) { (void)str; return str_len; }
int mMsg_Check_MainNormalContinue(mMsg_Window_c* msg_p) { (void)msg_p; return 0; }
int mMsg_Check_MainNormal(mMsg_Window_c* msg_p) { (void)msg_p; return 0; }
int mMsg_Check_MainHide(mMsg_Window_c* msg_p) { (void)msg_p; return 1; }
int mMsg_Check_MainDisappear(mMsg_Window_c* msg_p) { (void)msg_p; return 0; }
void mMsg_Set_CancelNormalContinue(mMsg_Window_c* msg_p) { (void)msg_p; }
void mMsg_Unset_CancelNormalContinue(mMsg_Window_c* msg_p) { (void)msg_p; }
void mMsg_Set_ForceNext(mMsg_Window_c* msg_p) { (void)msg_p; }
void mMsg_Unset_ForceNext(mMsg_Window_c* msg_p) { (void)msg_p; }
void mMsg_Set_LockContinue(mMsg_Window_c* msg_p) { (void)msg_p; }
void mMsg_Unset_LockContinue(mMsg_Window_c* msg_p) { (void)msg_p; }
void mMsg_Set_idling_req(mMsg_Window_c* msg_p) { (void)msg_p; }
int mMsg_Check_idling_now(mMsg_Window_c* msg_p) { (void)msg_p; return 0; }
int mMsg_sound_voice_get(int code) { (void)code; return 0; }
int mMsg_sound_voice_get_for_editor(int code) { (void)code; return 0; }
void mMsg_sound_spec_change_voice_force(mMsg_Window_c* msg_p) { (void)msg_p; }
int mMsg_sound_spec_change_voice(mMsg_Window_c* msg_p) { (void)msg_p; return 0; }
void mMsg_sound_set_voice_click(mMsg_Window_c* msg_p) { (void)msg_p; }
void mMsg_sound_set_voice_silent(mMsg_Window_c* msg_p, int update_mode) { (void)msg_p; (void)update_mode; }
void mMsg_sound_unset_voice_silent(mMsg_Window_c* msg_p, int update_mode) { (void)msg_p; (void)update_mode; }
void mMsg_debug_draw(gfxprint_t* gfxprint) { (void)gfxprint; }

/* ============================================================================
 * m_play.c additional stubs
 * ============================================================================ */

/* Forward declare types we need */
#include "PreRender.h"
#include "m_pause.h"
#include "libu64/pad.h"
#include "m_play.h"
#include "Famicom/famicom.h"
#include "dolphin/gx/GXFrameBuffer.h"

/* PreRender stubs */
void PreRender_cleanup(PreRender* render) { (void)render; }
void PreRender_init(PreRender* render) { (void)render; }
void PreRender_setup_savebuf(PreRender* render, u32 width, u32 height, void* fbuf, void* zbuf, void* cvg) {
    (void)render; (void)width; (void)height; (void)fbuf; (void)zbuf; (void)cvg;
}
void PreRender_setup_renderbuf(PreRender* render, u32 width, u32 height, void* fbuf, void* zbuf) {
    (void)render; (void)width; (void)height; (void)fbuf; (void)zbuf;
}

/* NPC */
void mNpc_ClearEventNpc(void) { }
void mNpc_ClearMaskNpc(void) { }

/* Player */
void mPlib_Object_Exchange_keep_Player_dt(GAME_PLAY* play) { (void)play; }
void mPlib_Clear_controller_data_for_title_demo(void) { }

/* Huusuiroom (feng shui room) */
void* mHsRm_GetHuusuiRoom(void) { return NULL; }

/* Field */
void mFM_Field_dt(void) { }
void mFM_FieldInit(GAME_PLAY* play) { (void)play; }
void mFI_ChangeClimate_ForEventNotice(GAME_PLAY* play) { (void)play; }

/* Memory card */
void mCD_toNextLand(void) { }
void mEA_CleanCardDLProgram(void) { }
void* mEA_GetCardDLProgram(void) { return NULL; }

/* Memory allocation - my_malloc_func is a struct, not a function */
static void* stub_malloc_align(size_t size, u32 align) { (void)align; return malloc(size); }
static void stub_free(void* ptr) { free(ptr); }
static int stub_getmemblocksize(void* ptr) { (void)ptr; return 0; }
static int stub_gettotalfreesize(void) { return 1024*1024; }
Famicom_MallocInfo my_malloc_func = {
    stub_malloc_align,
    stub_free,
    stub_getmemblocksize,
    stub_gettotalfreesize
};
void zelda_CleanupArena(void) { }
void zelda_InitArena(void** heap_start_p) { (void)heap_start_p; }

/* Time */
void mTM_time_init(void) { }

/* Pause */
void Pause_ct(pause_t* pause) { (void)pause; }
int Pause_proc(pause_t* pause, pad_t* pad) { (void)pause; (void)pad; return 0; }

/* Museum display */
void mMmd_MakeMuseumDisplayData(GAME_PLAY* play) { (void)play; }

/* Processing */
int none_proc1(void) { return 0; }

/* Balloon */
void Balloon_init(GAME_PLAY* play) { (void)play; }
void Balloon_move(GAME_PLAY* play) { (void)play; }

/* Notice */
void mNtc_set_auto_nwrite_data(void) { }

/* Vibration */
void mVibctl_clr_force_stop(void) { }
void mVibctl_set_force_stop(void) { }

/* Post office */
void mPO_business_proc(GAME_PLAY* play) { (void)play; }

/* Trash collection */
void mTRC_move(GAME_PLAY* play) { (void)play; }

/* Matrix operations */
void Matrix_MtxtoMtxF(Mtx* mtx, float mf[4][4]) { (void)mtx; (void)mf; }
void Skin_Matrix_MulMatrix(float mfA[4][4], float mfB[4][4], float mfOut[4][4]) {
    (void)mfA; (void)mfB; (void)mfOut;
}
void Matrix_reverse(float mf[4][4]) { (void)mf; }
void _MtxF_to_Mtx(float mf[4][4], Mtx* mtx) { (void)mf; (void)mtx; }

/* GameCube GX functions (used for texture copying) */
void GXBeginDisplayList(void* list, u32 size) { (void)list; (void)size; }
u32 GXEndDisplayList(void) { return 0; }
void GXSetTexCopySrc(u16 left, u16 top, u16 width, u16 height) {
    (void)left; (void)top; (void)width; (void)height;
}
void GXSetTexCopyDst(u16 wd, u16 ht, GXTexFmt fmt, GXBool mipmap) {
    (void)wd; (void)ht; (void)fmt; (void)mipmap;
}
void GXSetCopyFilter(GXBool aa, const u8 sample_pattern[12][2], GXBool vf, const u8 vfilter[7]) {
    (void)aa; (void)sample_pattern; (void)vf; (void)vfilter;
}
void GXCopyTex(void* dest, GXBool clear) { (void)dest; (void)clear; }

/* ============================================================================
 * JSystem / JKR File Loading (uses dvd_compat.c)
 * These functions wrap DVD I/O to load files from the disc image.
 * ============================================================================ */
#include "dolphin/dvd.h"
#include <stdio.h>

/* Expand switch values from jsyswrap */
#define EXPAND_SWITCH_NONE       0
#define EXPAND_SWITCH_DECOMPRESS 1
#define EXPAND_SWITCH_SZS        2

/* JC__JKRDvdToMainRam_byName - Load a file from DVD to RAM */
void* JC__JKRDvdToMainRam_byName(const char* filename, void* buffer, int expand_switch) {
    DVDFileInfo fileInfo;
    void* result = NULL;

    printf("[JKR] JC__JKRDvdToMainRam_byName(\"%s\", expand=%d)\n",
           filename ? filename : "(null)", expand_switch);

    if (!filename) {
        printf("[JKR]   ERROR: null filename\n");
        return NULL;
    }

    /* Open the file */
    if (!DVDOpen((char*)filename, &fileInfo)) {
        printf("[JKR]   ERROR: DVDOpen failed\n");
        return NULL;
    }

    /* Allocate buffer if not provided */
    size_t size = fileInfo.length;
    if (!buffer) {
        buffer = malloc(size + 32);  /* Extra space for alignment */
        if (!buffer) {
            printf("[JKR]   ERROR: malloc failed for %zu bytes\n", size);
            DVDClose(&fileInfo);
            return NULL;
        }
    }

    /* Read the file */
    s32 read = DVDReadPrio(&fileInfo, buffer, (s32)size, 0, 2);
    if (read < 0) {
        printf("[JKR]   ERROR: DVDReadPrio failed\n");
        free(buffer);
        DVDClose(&fileInfo);
        return NULL;
    }

    DVDClose(&fileInfo);

    /* TODO: Handle decompression (SZS, Yay0, etc.) if expand_switch != 0 */
    if (expand_switch != EXPAND_SWITCH_NONE) {
        printf("[JKR]   WARNING: decompression requested but not implemented\n");
        /* For now, return the raw data */
    }

    printf("[JKR]   SUCCESS: loaded %zu bytes\n", size);
    result = buffer;
    return result;
}

/* JKR Heap management - simplified for PC */
void* JKRGetCurrentHeap(void) {
    return NULL;  /* No heap management on PC */
}

void* JKRCreateExpHeap(size_t size, void* parent, int direction) {
    (void)size; (void)parent; (void)direction;
    return NULL;
}

void* JKRExpHeap_create(size_t size, void* parent, int direction) {
    (void)size; (void)parent; (void)direction;
    return NULL;
}

void* JKRAllocFromHeap(void* heap, size_t size, int alignment) {
    (void)heap; (void)alignment;
    return malloc(size);
}

void JKRFreeToHeap(void* heap, void* ptr) {
    (void)heap;
    free(ptr);
}

/* JKR Archive functions - stubbed for now */
void* JKRArchive_mountDvd(const char* path, int mount_mode, void* heap) {
    printf("[JKR] JKRArchive_mountDvd(\"%s\") - STUB\n", path ? path : "(null)");
    (void)path; (void)mount_mode; (void)heap;
    return NULL;
}

void* JKRArchive_get(void* archive, const char* path) {
    printf("[JKR] JKRArchive_get(\"%s\") - STUB\n", path ? path : "(null)");
    (void)archive; (void)path;
    return NULL;
}

/* JW (JSystem Wrapper) functions */
void* JW_JKRDvdToMainRam(const char* path, void* buffer, int expand) {
    return JC__JKRDvdToMainRam_byName(path, buffer, expand);
}

/* ============================================================================
 * JKR ARAM Archive System - Simplified for PC Port
 * On GameCube, archives were stored in ARAM (Audio RAM) to save main RAM.
 * On PC, we load them directly into main memory.
 * ============================================================================ */

/* Archive header structures (from JKRArchive) */
typedef struct {
    u32 magic;       /* 'RARC' */
    u32 fileSize;
    u32 headerSize;
    u32 dataOffset;
    u32 fileDataSize;
    u32 unknown1;
    u32 unknown2;
    u32 unknown3;
} RARCHeader;

/* Simple loaded archive tracking */
#define MAX_ARAM_ARCHIVES 8
typedef struct {
    void* data;         /* Raw archive data in memory */
    u32 size;           /* Size of archive */
    char path[256];     /* Path the archive was loaded from */
    int in_use;
} AramArchiveHandle;

static AramArchiveHandle aram_archives[MAX_ARAM_ARCHIVES];
static int aram_archives_init = 0;

static void aram_init(void) {
    if (!aram_archives_init) {
        memset(aram_archives, 0, sizeof(aram_archives));
        aram_archives_init = 1;
    }
}

static int find_free_aram_slot(void) {
    aram_init();
    for (int i = 0; i < MAX_ARAM_ARCHIVES; i++) {
        if (!aram_archives[i].in_use) return i;
    }
    return -1;
}

/* JC__JKRAramArchive_ctor - Create an ARAM archive handle */
void* JC__JKRAramArchive_ctor(void) {
    int slot = find_free_aram_slot();
    if (slot < 0) {
        printf("[JKR] JC__JKRAramArchive_ctor: no free slots\n");
        return NULL;
    }

    aram_archives[slot].in_use = 1;
    aram_archives[slot].data = NULL;
    aram_archives[slot].size = 0;
    aram_archives[slot].path[0] = '\0';

    printf("[JKR] JC__JKRAramArchive_ctor: slot %d\n", slot);
    return &aram_archives[slot];
}

/* JC__JKRMountFixedAramArchive - Mount an archive to "ARAM" (actually main RAM on PC) */
int JC__JKRMountFixedAramArchive(void* aram_archive, const char* filename) {
    AramArchiveHandle* handle = (AramArchiveHandle*)aram_archive;

    printf("[JKR] JC__JKRMountFixedAramArchive(\"%s\")\n", filename ? filename : "(null)");

    if (!handle || !filename) {
        printf("[JKR]   ERROR: null parameter\n");
        return 0;  /* FALSE */
    }

    /* Load the archive via DVD I/O */
    void* data = JC__JKRDvdToMainRam_byName(filename, NULL, EXPAND_SWITCH_NONE);
    if (!data) {
        printf("[JKR]   ERROR: failed to load archive\n");
        return 0;  /* FALSE */
    }

    /* Verify RARC magic (if it's a RARC archive) */
    RARCHeader* header = (RARCHeader*)data;
    if (header->magic == 0x52415243) {  /* 'RARC' */
        printf("[JKR]   RARC archive: size=%u bytes\n", header->fileSize);
    } else {
        printf("[JKR]   Non-RARC archive (magic=0x%08X)\n", header->magic);
    }

    handle->data = data;
    handle->size = header->fileSize;
    strncpy(handle->path, filename, sizeof(handle->path) - 1);

    printf("[JKR]   SUCCESS: archive mounted\n");
    return 1;  /* TRUE */
}

/* JC__JKRUnmountFixedAramArchive - Unmount an archive */
void JC__JKRUnmountFixedAramArchive(void* aram_archive) {
    AramArchiveHandle* handle = (AramArchiveHandle*)aram_archive;

    if (handle && handle->data) {
        free(handle->data);
        handle->data = NULL;
        handle->in_use = 0;
        printf("[JKR] JC__JKRUnmountFixedAramArchive: unmounted\n");
    }
}

/* JC_JKRAramArchive_getAramAddress_byName - Get address of resource in archive */
u32 JC_JKRAramArchive_getAramAddress_byName(void* archive, u32 root_name, const char* res_name) {
    AramArchiveHandle* handle = (AramArchiveHandle*)archive;

    printf("[JKR] getAramAddress_byName(root=0x%X, res=\"%s\")\n",
           root_name, res_name ? res_name : "(null)");

    if (!handle || !handle->data) {
        printf("[JKR]   ERROR: archive not mounted\n");
        return 0;
    }

    /* TODO: Implement proper RARC resource lookup */
    /* For now, return the base data address as a placeholder */
    printf("[JKR]   WARNING: resource lookup not fully implemented\n");

    return (u32)(uintptr_t)handle->data;
}

/* JC__JKRAllocFromAram - Allocate from "ARAM" (uses main RAM on PC) */
void* JC__JKRAllocFromAram(size_t size) {
    void* ptr = malloc(size);
    printf("[JKR] JC__JKRAllocFromAram(%zu) = %p\n", size, ptr);
    return ptr;
}

/* JC__JKRAramToMainRam_block - Copy from ARAM to main RAM (no-op on PC) */
u8* JC__JKRAramToMainRam_block(void* aramBlock, u8* ramDst, size_t size) {
    /* On PC, ARAM IS main RAM, so just memcpy */
    if (aramBlock && ramDst) {
        memcpy(ramDst, aramBlock, size);
    }
    return ramDst;
}

/* JC__JKRMainRamToAram_block - Copy from main RAM to ARAM (no-op on PC) */
void* JC__JKRMainRamToAram_block(u8* ramAddr, void* aramBlock, size_t size) {
    if (ramAddr && aramBlock) {
        memcpy(aramBlock, ramAddr, size);
    }
    return aramBlock;
}

/* JC_JKRAramBlock_getAddress - Get address of ARAM block */
u32 JC_JKRAramBlock_getAddress(void* aramBlock) {
    return (u32)(uintptr_t)aramBlock;
}

/* JC_JKRAramArchive_new - Create a new ARAM archive handle (alias for ctor) */
void* JC_JKRAramArchive_new(void) {
    return JC__JKRAramArchive_ctor();
}

/* JC_JKRAramArchive_delete - Delete an ARAM archive handle */
void JC_JKRAramArchive_delete(void* archive) {
    AramArchiveHandle* handle = (AramArchiveHandle*)archive;
    if (handle) {
        if (handle->data) {
            free(handle->data);
            handle->data = NULL;
        }
        handle->in_use = 0;
    }
}

/* ============================================================================
 * JW_Init2 / JW_Init3 - Load game archives
 * These are called during boot to load the main game resource archives.
 * ============================================================================ */

/* Global archive handles */
static void* forest_arc_aram_p = NULL;
static void* forest_arc_aram2_p = NULL;

/* JW_Init2 - Load first archive (forest_1st.arc) */
void JW_Init2(void) {
    printf("[JW] JW_Init2() - Loading forest_1st.arc\n");

    if (forest_arc_aram_p == NULL) {
        forest_arc_aram_p = JC_JKRAramArchive_new();

        if (forest_arc_aram_p == NULL ||
            JC__JKRMountFixedAramArchive(forest_arc_aram_p, "forest_1st.arc") == 0) {
            printf("[JW] ERROR: Failed to load forest_1st.arc\n");
            /* Don't call OSDVDFatalError - just continue on PC */
        } else {
            printf("[JW] Successfully loaded forest_1st.arc\n");
        }
    }
}

/* JW_Init3 - Load second archive (forest_2nd.arc) */
void JW_Init3(void) {
    printf("[JW] JW_Init3() - Loading forest_2nd.arc\n");

    if (forest_arc_aram2_p == NULL) {
        forest_arc_aram2_p = JC_JKRAramArchive_new();

        if (forest_arc_aram2_p == NULL ||
            JC__JKRMountFixedAramArchive(forest_arc_aram2_p, "forest_2nd.arc") == 0) {
            printf("[JW] ERROR: Failed to load forest_2nd.arc\n");
            /* Don't call OSDVDFatalError - just continue on PC */
        } else {
            printf("[JW] Successfully loaded forest_2nd.arc\n");
        }
    }
}

/* Build date/version info */
void* boot_copyDate = (void*)"PC Port";
const char* __DateTime__ = __DATE__ " " __TIME__;
const char* __Creator__ = "PC Port";

/* Memory variable needed by m_play.c */
void* my_malloc_current = NULL;

/* Additional m_play.c dependencies */
#include "m_controller.h"
#include "m_lights.h"
void mCon_main(GAME* game) { (void)game; }
void Global_light_ct(Global_light* glight) { (void)glight; }

/* ============================================================================
 * Scene info stubs - these are the Scene_Word_u structures for each game scene
 * They are declared as arrays in m_scene.h, so we define them as single-element arrays
 * ============================================================================ */
#include "m_scene.h"

/* Dummy scene data with proper END marker */
Scene_Word_u test01_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u test02_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u test03_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u water_test_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u test_step01_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u test04_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u npc_room01_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u test_fd_npc_land_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u field_tool_field_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u shop01_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u BG_TEST01_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u BG_TEST01_XLU_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u broker_shop_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u fg_tool_in_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u post_office_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u start_demo1_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u start_demo2_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u police_box_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u buggy_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u player_select_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u player_room_s_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u player_room_m_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u player_room_l_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u shop02_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u shop03_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u shop04_1f_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u test05_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u PLAYER_SELECT2_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u PLAYER_SELECT3_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u shop04_2f_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u event_notification_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u kamakura_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u title_demo_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u PLAYER_SELECT4_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u museum_entrance_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u museum_picture_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u museum_fossil_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u museum_insect_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u museum_fish_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u player_room_ll1_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u player_room_ll2_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u tailor_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u NEEDLEWORK_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u p_room_bm_s_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u p_room_bm_m_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u p_room_bm_l_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u p_room_bm_ll1_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u player_room_island_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u npc_room_island_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u start_demo3_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u lighthouse_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };
Scene_Word_u tent_info[] = { { .misc = { mSc_SCENE_DATA_TYPE_END, 0, 0, 0, 0 } } };

/*
 * NOTE: Additional function stubs are in linker_stubs.c which is compiled without
 * including conflicting headers to avoid type mismatches.
 */
