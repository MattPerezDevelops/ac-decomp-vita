#include "graph.h"

#ifdef TARGET_PC
#include <stdio.h>
#endif

#include "audio.h"
#include "dvderr.h"
#include "famicom_emu.h"
#include "first_game.h"
#include "game.h"
#include "irqmgr.h"
#include "libc64/malloc.h"
#include "libforest/emu64/emu64_wrapper.h"
#include "jsyswrap.h"
#include "libu64/debug.h"
#include "libultra/libultra.h"
#include "m_bgm.h"
#include "m_debug.h"
#include "m_game_dlftbls.h"
#include "m_play.h"
#include "m_prenmi.h"
#include "m_select.h"
#include "m_trademark.h"
#include "m_vibctl.h"
#include "player_select.h"
#include "save_menu.h"
#include "second_game.h"
#include "sys_dynamic.h"
#include "sys_ucode.h"
#include "zurumode.h"

GRAPH graph_class;

static int skip_frame; // TODO: this is actually declared in graph_main
#if VERSION != VER_GAFU01_00
u8 SoftResetEnable;
#endif
static int frame; // TODO: this is actually declared in graph_task_set00

#ifdef TARGET_PC
/* GCC-compatible version (Metrowerks allows token pasting with . but GCC doesn't) */
#define CONSTRUCT_THA_GA(tha_ga, name, name2) (THA_GA_ct((tha_ga), sys_dynamic.name, name2##_SIZE * sizeof(Gfx)))
#else
/* Original Metrowerks version */
#define CONSTRUCT_THA_GA(tha_ga, name, name2) (THA_GA_ct((tha_ga), sys_dynamic.##name, ##name2##_SIZE * sizeof(Gfx)))
#endif

static void graph_setup_double_buffer(GRAPH* this) {
    bzero(&sys_dynamic, sizeof(dynamic_t));
    sys_dynamic.start_magic = SYSDYNAMIC_START_MAGIC;
    sys_dynamic.end_magic = SYSDYNAMIC_END_MAGIC;

#ifdef TARGET_PC
    /* Initialize each display list buffer with G_ENDDL terminator.
     * This prevents infinite recursion when game states don't render anything,
     * since empty buffers (all zeros = G_NOOP) would otherwise be traversed
     * indefinitely when following branch chains. */
    sys_dynamic.new0[0].words.w0 = (u32)G_ENDDL << 24;
    sys_dynamic.new1[0].words.w0 = (u32)G_ENDDL << 24;
    sys_dynamic.poly_opa[0].words.w0 = (u32)G_ENDDL << 24;
    sys_dynamic.poly_xlu[0].words.w0 = (u32)G_ENDDL << 24;
    sys_dynamic.overlay[0].words.w0 = (u32)G_ENDDL << 24;
    sys_dynamic.work[0].words.w0 = (u32)G_ENDDL << 24;
    sys_dynamic.font[0].words.w0 = (u32)G_ENDDL << 24;
    sys_dynamic.shadow[0].words.w0 = (u32)G_ENDDL << 24;
    sys_dynamic.light[0].words.w0 = (u32)G_ENDDL << 24;
#endif

    CONSTRUCT_THA_GA(&this->bg_opaque_thaga, new0, NEW0);
    CONSTRUCT_THA_GA(&this->bg_translucent_thaga, new1, NEW1);
    CONSTRUCT_THA_GA(&this->polygon_opaque_thaga, poly_opa, POLY_OPA);
    CONSTRUCT_THA_GA(&this->polygon_translucent_thaga, poly_xlu, POLY_XLU);
    CONSTRUCT_THA_GA(&this->overlay_thaga, overlay, OVERLAY);
    CONSTRUCT_THA_GA(&this->work_thaga, work, WORK);
    CONSTRUCT_THA_GA(&this->font_thaga, font, FONT);
    CONSTRUCT_THA_GA(&this->shadow_thaga, shadow, SHADOW);
    CONSTRUCT_THA_GA(&this->light_thaga, light, LIGHT);

    this->Gfx_list10 = sys_dynamic.new0;
    this->Gfx_list11 = sys_dynamic.new1;
    this->Gfx_list00 = sys_dynamic.poly_opa;
    this->Gfx_list01 = sys_dynamic.poly_xlu;
    this->Gfx_list04 = sys_dynamic.overlay;
    this->Gfx_list05 = sys_dynamic.work;
    this->Gfx_list07 = sys_dynamic.font;
    this->Gfx_list08 = sys_dynamic.shadow;
    this->Gfx_list09 = sys_dynamic.light;

#ifdef TARGET_PC
    static int buf_debug = 0;
    if (buf_debug++ < 3) {
        printf("[SETUP] frame=%d ALL BUFFERS:\n", buf_debug);
        printf("  work=%p new0=%p shadow=%p\n", (void*)sys_dynamic.work, (void*)sys_dynamic.new0, (void*)sys_dynamic.shadow);
        printf("  new1=%p poly_opa=%p poly_xlu=%p\n", (void*)sys_dynamic.new1, (void*)sys_dynamic.poly_opa, (void*)sys_dynamic.poly_xlu);
        printf("  light=%p font=%p overlay=%p\n", (void*)sys_dynamic.light, (void*)sys_dynamic.font, (void*)sys_dynamic.overlay);
        if (buf_debug == 1) {
            printf("[SIZES] GRAPH=%zu THA_GA=%zu Gfx*=%zu\n", sizeof(GRAPH), sizeof(THA_GA), sizeof(Gfx*));
            printf("[SIZES] polygon_opaque_thaga offset: %zu (expected 0x2C8=712)\n",
                   (size_t)((char*)&this->polygon_opaque_thaga - (char*)this));
            printf("[SIZES] tail_p=%p buf_p=%p\n",
                   (void*)this->polygon_opaque_thaga.tha.tail_p,
                   (void*)this->polygon_opaque_thaga.thaGfx.buf_p);
        }
        fflush(stdout);
    }
#endif

    this->gfxsave = NULL;
}

#define ARE_INIT_PROCS_EQUAL(proc0, proc1) (((void (*)(GAME*))proc0) == ((void (*)(GAME*))proc1))
static DLFTBL_GAME* game_get_next_game_dlftbl(GAME* game) {
    void (*next_game_init_proc)(GAME*) = game_get_next_game_init(game);

    if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, first_game_init)) {
        return &game_dlftbls[0];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, select_init)) {
        return &game_dlftbls[1];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, play_init)) {
        return &game_dlftbls[2];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, second_game_init)) {
        return &game_dlftbls[3];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, trademark_init)) {
        return &game_dlftbls[5];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, player_select_init)) {
        return &game_dlftbls[6];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, save_menu_init)) {
        return &game_dlftbls[7];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, famicom_emu_init)) {
        return &game_dlftbls[8];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, prenmi_init)) {
        return &game_dlftbls[9];
    }

    return NULL;
}

extern void graph_ct(GRAPH* this) {
    bzero(this, sizeof(GRAPH));
    this->frame_counter = 0;
    this->cfb_bank = 0;
    SETREG(SREG, 33, GETREG(SREG, 33) & ~2);
    SETREG(SREG, 33, GETREG(SREG, 33) & ~1);
    zurumode_init();
    GRAPH_SET_DOING_POINT(this, CT);
}

extern void graph_dt(GRAPH* this) {
    GRAPH_SET_DOING_POINT(this, DT);
    zurumode_cleanup();
}

static void graph_task_set00(GRAPH* this) {
    ucode_info ucode[2];

#ifdef TARGET_PC
    printf("[graph_task_set00] Entry\n"); fflush(stdout);
#endif
    GRAPH_SET_DOING_POINT(this, WAIT_TASK);
    GRAPH_SET_DOING_POINT(this, WAIT_TASK_FINISHED);
    if (ResetStatus < IRQ_RESET_DELAY) {
        this->last_dl = this->Gfx_list05;
        if (this->taskEndCallback != NULL) {
            this->taskEndCallback(this, this->taskEndData);
        }

        if (ResetStatus < IRQ_RESET_DELAY) {
#ifdef TARGET_PC
            printf("[graph_task_set00] Setting up ucode\n"); fflush(stdout);
#endif
            ucode[0].type = UCODE_TYPE_POLY_TEXT;
            ucode[1].type = UCODE_TYPE_SPRITE_TEXT;
            ucode[0].ucode_p = ucode_GetPolyTextStart();
            ucode[1].ucode_p = ucode_GetSpriteTextStart();
#ifdef TARGET_PC
            printf("[graph_task_set00] JW_BeginFrame\n"); fflush(stdout);
#endif
            JW_BeginFrame();
#ifdef TARGET_PC
            printf("[graph_task_set00] emu64_init\n"); fflush(stdout);
#endif
            emu64_init();
            emu64_set_ucode_info(2, ucode);
            emu64_set_first_ucode(ucode[0].ucode_p);
#ifdef TARGET_PC
            printf("[graph_task_set00] emu64_taskstart with Gfx_list05=%p\n", (void*)this->Gfx_list05); fflush(stdout);
#endif
            emu64_taskstart(this->Gfx_list05); /* work data */
#ifdef TARGET_PC
            printf("[graph_task_set00] emu64_cleanup\n"); fflush(stdout);
#endif
            emu64_cleanup();
            JW_EndFrame();
            frame++;
        }
    }
#ifdef TARGET_PC
    printf("[graph_task_set00] Done\n"); fflush(stdout);
#endif
}

static int graph_draw_finish(GRAPH* this) {
    int err;
    OPEN_DISP(this);

#ifdef TARGET_PC
    static int finish_debug = 0;
    if (finish_debug++ < 5) {
        printf("[BEFORE_FINISH] frame=%d poly_opa[0]=(0x%08X,0x%08X) head=%p buf=%p\n",
               finish_debug, sys_dynamic.poly_opa[0].words.w0, sys_dynamic.poly_opa[0].words.w1,
               (void*)NOW_POLY_OPA_DISP, (void*)sys_dynamic.poly_opa);
        fflush(stdout);
    }
#endif

    gSPBranchList(NOW_WORK_DISP++, this->Gfx_list10);
    gSPBranchList(NOW_BG_OPA_DISP++, this->Gfx_list08);
    gSPBranchList(NOW_SHADOW_DISP++, this->Gfx_list11);
    gSPBranchList(NOW_BG_XLU_DISP++, this->Gfx_list00);
    gSPBranchList(NOW_POLY_OPA_DISP++, this->Gfx_list01);
    gSPBranchList(NOW_POLY_XLU_DISP++, this->Gfx_list09);
    gSPBranchList(NOW_LIGHT_DISP++, this->Gfx_list07);
    gSPBranchList(NOW_FONT_DISP++, this->Gfx_list04);
    gDPPipeSync(NOW_OVERLAY_DISP++);
    gDPFullSync(NOW_OVERLAY_DISP++);
    gSPEndDisplayList(NOW_OVERLAY_DISP++);

#ifdef TARGET_PC
    /* Debug: dump BG_OPA buffer contents after linking */
    static int chain_debug = 0;
    chain_debug++;
    /* Show gameplay frame 185 where player cube should be */
    if (chain_debug == 185) {
        printf("\n[DL_CHAIN] frame=%d Buffer contents after linking:\n", chain_debug);
        printf("  BUFFER ADDRESSES:\n");
        printf("    work=%p (Gfx_list05=%p)\n", (void*)sys_dynamic.work, (void*)this->Gfx_list05);
        printf("    new0=%p (Gfx_list10=%p) <- BG_OPA\n", (void*)sys_dynamic.new0, (void*)this->Gfx_list10);
        printf("    shadow=%p (Gfx_list08=%p)\n", (void*)sys_dynamic.shadow, (void*)this->Gfx_list08);
        printf("    poly_opa=%p (Gfx_list00=%p)\n", (void*)sys_dynamic.poly_opa, (void*)this->Gfx_list00);
        printf("  work[0]: op=0x%02X (0x%08X, 0x%08X)\n",
               sys_dynamic.work[0].words.w0 >> 24, sys_dynamic.work[0].words.w0, sys_dynamic.work[0].words.w1);
        printf("  new0[0]: op=0x%02X (0x%08X, 0x%08X) <- BG_OPA start\n",
               sys_dynamic.new0[0].words.w0 >> 24, sys_dynamic.new0[0].words.w0, sys_dynamic.new0[0].words.w1);
        printf("  new0[1]: op=0x%02X (0x%08X, 0x%08X)\n",
               sys_dynamic.new0[1].words.w0 >> 24, sys_dynamic.new0[1].words.w0, sys_dynamic.new0[1].words.w1);
        printf("  new0[2]: op=0x%02X (0x%08X, 0x%08X)\n",
               sys_dynamic.new0[2].words.w0 >> 24, sys_dynamic.new0[2].words.w0, sys_dynamic.new0[2].words.w1);
        printf("  new0[3]: op=0x%02X (0x%08X, 0x%08X)\n",
               sys_dynamic.new0[3].words.w0 >> 24, sys_dynamic.new0[3].words.w0, sys_dynamic.new0[3].words.w1);
        printf("  new0[4]: op=0x%02X (0x%08X, 0x%08X)\n",
               sys_dynamic.new0[4].words.w0 >> 24, sys_dynamic.new0[4].words.w0, sys_dynamic.new0[4].words.w1);
        printf("  new0[5]: op=0x%02X (0x%08X, 0x%08X)\n",
               sys_dynamic.new0[5].words.w0 >> 24, sys_dynamic.new0[5].words.w0, sys_dynamic.new0[5].words.w1);
        printf("  new0[6]: op=0x%02X (0x%08X, 0x%08X)\n",
               sys_dynamic.new0[6].words.w0 >> 24, sys_dynamic.new0[6].words.w0, sys_dynamic.new0[6].words.w1);
        printf("  new0[10]: op=0x%02X (0x%08X, 0x%08X)\n",
               sys_dynamic.new0[10].words.w0 >> 24, sys_dynamic.new0[10].words.w0, sys_dynamic.new0[10].words.w1);
        printf("  new0[11]: op=0x%02X (0x%08X, 0x%08X)\n",
               sys_dynamic.new0[11].words.w0 >> 24, sys_dynamic.new0[11].words.w0, sys_dynamic.new0[11].words.w1);
        printf("  new0[12]: op=0x%02X (0x%08X, 0x%08X) <- aFD_DrawBg should write here\n",
               sys_dynamic.new0[12].words.w0 >> 24, sys_dynamic.new0[12].words.w0, sys_dynamic.new0[12].words.w1);
        printf("  new0[13]: op=0x%02X (0x%08X, 0x%08X)\n",
               sys_dynamic.new0[13].words.w0 >> 24, sys_dynamic.new0[13].words.w0, sys_dynamic.new0[13].words.w1);
        printf("  new0[14]: op=0x%02X (0x%08X, 0x%08X)\n",
               sys_dynamic.new0[14].words.w0 >> 24, sys_dynamic.new0[14].words.w0, sys_dynamic.new0[14].words.w1);
        /* How many commands were written to BG_OPA? */
        Gfx* bg_opa_head = (Gfx*)this->bg_opaque_thaga.thaGfx.head_p;
        long cmds_written = (long)(bg_opa_head - this->Gfx_list10);
        printf("  BG_OPA head=%p, start=%p, cmds written=%ld\n",
               (void*)bg_opa_head, (void*)this->Gfx_list10, cmds_written);
        /* Dump commands around where player cube should be (near head) */
        if (cmds_written > 45) {
            printf("  new0[%ld]: op=0x%02X (0x%08X, 0x%08X) <- cube should be near here\n",
                   cmds_written-2, sys_dynamic.new0[cmds_written-2].words.w0 >> 24,
                   sys_dynamic.new0[cmds_written-2].words.w0, sys_dynamic.new0[cmds_written-2].words.w1);
            printf("  new0[%ld]: op=0x%02X (0x%08X, 0x%08X)\n",
                   cmds_written-1, sys_dynamic.new0[cmds_written-1].words.w0 >> 24,
                   sys_dynamic.new0[cmds_written-1].words.w0, sys_dynamic.new0[cmds_written-1].words.w1);
            printf("  new0[%ld]: op=0x%02X (0x%08X, 0x%08X) <- head (BranchList)\n",
                   cmds_written, sys_dynamic.new0[cmds_written].words.w0 >> 24,
                   sys_dynamic.new0[cmds_written].words.w0, sys_dynamic.new0[cmds_written].words.w1);
        }
        fflush(stdout);
    }
#endif

#ifdef TARGET_PC
    if (finish_debug <= 5) {
        printf("[AFTER_FINISH] poly_opa[0]=(0x%08X,0x%08X)\n",
               sys_dynamic.poly_opa[0].words.w0, sys_dynamic.poly_opa[0].words.w1);
        fflush(stdout);
    }
#endif

    CLOSE_DISP(this);
    err = FALSE;

    SYSDYNAMIC_OPEN();
    if (!SYSDYNAMIC_CHECK_START()) {
#if VERSION == VER_GAFU01_00
        _dbg_hungup(__FILE__, 416);
#elif VERSION == VER_GAFE01_00
        _dbg_hungup(__FILE__, 417);
#endif
    }

    if (!SYSDYNAMIC_CHECK_END()) {
        err = TRUE;
#if VERSION == VER_GAFU01_00
        _dbg_hungup(__FILE__, 424);
#elif VERSION == VER_GAFE01_00
        _dbg_hungup(__FILE__, 425);
#endif
    }
    SYSDYNAMIC_CLOSE();

    if (THA_GA_isCrash(&this->polygon_opaque_thaga)) {
        err = TRUE;
    }

    if (THA_GA_isCrash(&this->polygon_translucent_thaga)) {
        err = TRUE;
    }

    if (THA_GA_isCrash(&this->overlay_thaga)) {
        err = TRUE;
    }

    if (THA_GA_isCrash(&this->font_thaga)) {
        err = TRUE;
    }

    if (THA_GA_isCrash(&this->shadow_thaga)) {
        err = TRUE;
    }

    if (THA_GA_isCrash(&this->light_thaga)) {
        err = TRUE;
    }

    if (THA_GA_isCrash(&this->bg_opaque_thaga)) {
        err = TRUE;
    }

    if (THA_GA_isCrash(&this->bg_translucent_thaga)) {
        err = TRUE;
    }

    return err;
}

static void do_soft_reset(GAME* game) {
    SoftResetEnable = FALSE;
    mBGM_reset();
    mVibctl_reset();
    sAdo_SoftReset();
    ResetTime = osGetTime();
    ResetStatus = IRQ_RESET_PRENMI;
}

static void reset_check(GRAPH* this, GAME* game) {
    if (SoftResetEnable && osShutdown) {
        do_soft_reset(game);
    }
}

// Aus version removes debug frame skip logic
#if VERSION >= VER_GAFU01_00
static void graph_main(GRAPH* this, GAME* game) {
#ifdef TARGET_PC
    printf("[graph_main] Starting\n"); fflush(stdout);
#endif
    game->disable_prenmi = FALSE;
#ifdef TARGET_PC
    printf("[graph_main] graph_setup_double_buffer\n"); fflush(stdout);
#endif
    graph_setup_double_buffer(this);
#ifdef TARGET_PC
    printf("[graph_main] game_get_controller\n"); fflush(stdout);
#endif
    game_get_controller(game);
    game->disable_display = FALSE;
    GRAPH_SET_DOING_POINT(this, GAME_MAIN);
#ifdef TARGET_PC
    printf("[graph_main] Calling game_main()\n"); fflush(stdout);
#endif
    game_main(game);
#ifdef TARGET_PC
    printf("[graph_main] game_main() returned\n"); fflush(stdout);
#endif
    GRAPH_SET_DOING_POINT(this, GAME_MAIN_FINISHED);
    if (ResetStatus < IRQ_RESET_DELAY) {
        if (game->disable_display == FALSE) {
            if (graph_draw_finish(this) == FALSE) {
                GRAPH_SET_DOING_POINT(this, TASK_SET);
                graph_task_set00(this);
                GRAPH_SET_DOING_POINT(this, TASK_SET_FINISHED);
                this->frame_counter++;

                if ((GETREG(SREG, 33) & 1) != 0) {
                    SETREG(SREG, 33, GETREG(SREG, 33) & ~1);
                }
            }
        }
    }

    if (GETREG(SREG, 20) < 2) {
        GRAPH_SET_DOING_POINT(this, AUDIO);
        sAdo_GameFrame();
        GRAPH_SET_DOING_POINT(this, AUDIO_FINISHED);
    }

    reset_check(this, game);

    if (ResetStatus == IRQ_RESET_PRENMI && game->disable_prenmi == FALSE) {
        GAME_GOTO_NEXT(game, prenmi, PRENMI);
    }
}
#else
static void graph_main(GRAPH* this, GAME* game) {
#ifdef TARGET_PC
    printf("[graph_main-v0] Starting\n"); fflush(stdout);
#endif
    game->disable_prenmi = FALSE;
#ifdef TARGET_PC
    printf("[graph_main-v0] graph_setup_double_buffer\n"); fflush(stdout);
#endif
    graph_setup_double_buffer(this);
#ifdef TARGET_PC
    printf("[graph_main-v0] game_get_controller\n"); fflush(stdout);
#endif
    game_get_controller(game);
#ifdef TARGET_PC
    printf("[graph_main-v0] game_get_controller done\n"); fflush(stdout);
#endif
    game->disable_display = FALSE;
    GRAPH_SET_DOING_POINT(this, GAME_MAIN);
#ifdef TARGET_PC
    printf("[graph_main-v0] Calling game_main\n"); fflush(stdout);
#endif
    game_main(game);
#ifdef TARGET_PC
    printf("[graph_main-v0] game_main done\n"); fflush(stdout);
    static int after_game_debug = 0;
    if (after_game_debug++ < 5) {
        printf("[AFTER_GAME] frame=%d poly_opa[0]=(0x%08X,0x%08X) head=%p buf=%p\n",
               after_game_debug, sys_dynamic.poly_opa[0].words.w0, sys_dynamic.poly_opa[0].words.w1,
               (void*)this->polygon_opaque_thaga.thaGfx.head_p, (void*)sys_dynamic.poly_opa);
        fflush(stdout);
    }
#endif
    GRAPH_SET_DOING_POINT(this, GAME_MAIN_FINISHED);
#ifdef TARGET_PC
    printf("[graph_main-v0] Checking ResetStatus=%d\n", ResetStatus); fflush(stdout);
#endif
    if (ResetStatus < IRQ_RESET_DELAY) {
        if (skip_frame < GETREG(SREG, 3)) {
            skip_frame++;
            this->frame_counter++;
        } else if (game->disable_display == FALSE) {
            skip_frame = 0;
#ifdef TARGET_PC
            printf("[graph_main-v0] Calling graph_draw_finish\n"); fflush(stdout);
#endif
            if (graph_draw_finish(this) == FALSE) {
#ifdef TARGET_PC
                printf("[graph_main-v0] graph_draw_finish returned, calling graph_task_set00\n"); fflush(stdout);
#endif
                GRAPH_SET_DOING_POINT(this, TASK_SET);
                graph_task_set00(this);
                GRAPH_SET_DOING_POINT(this, TASK_SET_FINISHED);
                this->frame_counter++;

                if ((GETREG(SREG, 33) & 1) != 0) {
                    SETREG(SREG, 33, GETREG(SREG, 33) & ~1);
                }
            }
        }
    }

#ifdef TARGET_PC
    printf("[graph_main-v0] Audio section\n"); fflush(stdout);
#endif
    if (GETREG(SREG, 20) < 2) {
        GRAPH_SET_DOING_POINT(this, AUDIO);
        sAdo_GameFrame();
        GRAPH_SET_DOING_POINT(this, AUDIO_FINISHED);
    }

#ifdef TARGET_PC
    printf("[graph_main-v0] reset_check\n"); fflush(stdout);
#endif
    reset_check(this, game);

    if (ResetStatus == IRQ_RESET_PRENMI && game->disable_prenmi == FALSE) {
        GAME_GOTO_NEXT(game, prenmi, PRENMI);
    }
#ifdef TARGET_PC
    printf("[graph_main-v0] Done!\n"); fflush(stdout);
#endif
}
#endif

extern void graph_proc(void* arg) {
    GRAPH* __graph = &graph_class;
    DLFTBL_GAME* dlftbl = &game_dlftbls[0];

#ifdef TARGET_PC
    printf("[GRAPH] graph_proc() starting\n");
    printf("[GRAPH] dlftbl = %p\n", (void*)dlftbl);
    fflush(stdout);
#endif

    graph_ct(&graph_class);

#ifdef TARGET_PC
    printf("[GRAPH] graph_ct() done\n");
    fflush(stdout);
#endif

#ifdef TARGET_PC
    extern int window_should_close(void);
    while (dlftbl != NULL && !window_should_close()) {
#else
    while (dlftbl != NULL) {
#endif
        size_t size = dlftbl->alloc_size;
#ifdef TARGET_PC
        printf("[GRAPH] Allocating game state, size = %zu\n", size);
        fflush(stdout);
#endif
        GAME* game = (GAME*)malloc(size);
        if (!game) {
#ifdef TARGET_PC
            printf("[GRAPH] ERROR: malloc failed!\n");
            fflush(stdout);
#endif
            return;
        }
        game_class_p = game;
        bzero(game, size);
        GRAPH_SET_DOING_POINT(__graph, GAME_CT);
#ifdef TARGET_PC
        printf("[GRAPH] Calling game_ct()\n");
        fflush(stdout);
#endif
        game_ct(game, dlftbl->init, __graph);
#ifdef TARGET_PC
        printf("[GRAPH] game_ct() done\n");
        fflush(stdout);
#endif
        emu64_refresh();
        GRAPH_SET_DOING_POINT(__graph, GAME_CT_FINISHED);

#ifdef TARGET_PC
        printf("[GRAPH] Entering game loop\n");
        fflush(stdout);
#endif
        while (game_is_doing(game)) {
#ifdef TARGET_PC
            printf("[GRAPH] game_is_doing=true, calling dvderr_draw\n");
            fflush(stdout);
#endif
            if (!dvderr_draw()) {
#ifdef TARGET_PC
                printf("[GRAPH] Calling graph_main, __graph=%p, game=%p\n", (void*)__graph, (void*)game);
                printf("[GRAPH] game->exec=%p\n", (void*)game->exec);
                fflush(stdout);
#endif
                graph_main(__graph, game);
#ifdef TARGET_PC
                printf("[GRAPH] graph_main returned\n");
                fflush(stdout);
#endif
            }
        }
#ifdef TARGET_PC
        printf("[GRAPH] Exited game loop\n");
        fflush(stdout);
#endif

        dlftbl = game_get_next_game_dlftbl(game);
        GRAPH_SET_DOING_POINT(__graph, GAME_18);
        GRAPH_SET_DOING_POINT(__graph, GAME_DT);
        game_dt(game);
        GRAPH_SET_DOING_POINT(__graph, GAME_DT_FINISHED);
        free(game);
        game_class_p = NULL;
    }

    graph_dt(__graph);
}
