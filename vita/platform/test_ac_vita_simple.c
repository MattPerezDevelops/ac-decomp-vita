/**
 * AC-Decomp Vita Wrapper Demo
 * Shows how AC-Decomp code works without modification using our wrapper
 * 
 * This is an example of how ANY AC-Decomp source file can be compiled
 * for Vita with minimal changes - just include our wrapper!
 */

#include "ac_vita_platform.h"

// Example: This looks exactly like AC-Decomp code!
// But it runs on Vita thanks to our wrapper.

// Asset declarations (these would normally be #include "assets/...")
// Our wrapper automatically loads them at runtime
ASSET_TEXTURE_ARRAY(kan_win_map_tex);
ASSET_PALETTE_ARRAY(req_win_kao_pal);

// Example game state structure (typical AC style)
typedef struct {
    f32 player_x, player_y, player_z;
    u16 current_buttons;
    u32 frame_count;
    BOOL is_running;
} game_state_t;

static game_state_t g_game_state = {0};

// Example functions using GameCube APIs
void render_simple_quad(void) {
    // This is pure GameCube GX code, but runs on VitaGL!
    GXBegin(GX_QUADS, 0, 4);
    
    GXColor4u8(255, 255, 255, 255);
    GXPosition2f32(-100.0f, -100.0f);
    GXTexCoord2f32(0.0f, 0.0f);
    
    GXPosition2f32(100.0f, -100.0f);
    GXTexCoord2f32(1.0f, 0.0f);
    
    GXPosition2f32(100.0f, 100.0f);
    GXTexCoord2f32(1.0f, 1.0f);
    
    GXPosition2f32(-100.0f, 100.0f);
    GXTexCoord2f32(0.0f, 1.0f);
    
    GXEnd();
}

void update_player_input(void) {
    PADStatus pad_status;
    
    // Read controller - looks like GameCube code!
    if (PADRead(&pad_status)) {
        g_game_state.current_buttons = pad_status.button;
        
        // Move player based on analog stick
        if (pad_status.stickX != 0 || pad_status.stickY != 0) {
            g_game_state.player_x += pad_status.stickX * 0.1f;
            g_game_state.player_y += pad_status.stickY * 0.1f;
        }
        
        // Exit on START button
        if (pad_status.button & PAD_BUTTON_START) {
            g_game_state.is_running = FALSE;
        }
    }
}

void game_main_loop(void) {
    printf("🎮 Starting Animal Crossing Vita Demo...\n");
    
    // Set up viewport (GameCube style!)
    GXSetViewport(0, 0, 960, 544, 0.0f, 1.0f);
    
    g_game_state.is_running = TRUE;
    g_game_state.player_x = 0.0f;
    g_game_state.player_y = 0.0f;
    g_game_state.frame_count = 0;
    
    while (g_game_state.is_running) {
        // Begin frame (our wrapper handles VitaGL setup)
        AC_Vita_Frame_Begin();
        
        // Update game logic
        update_player_input();
        
        // Clear screen
        GXColor clear_color = {135, 206, 240, 255}; // Light blue
        GXSetCopyClear(clear_color, 0xFFFFFF);
        
        // Set up camera matrix
        f32 view_matrix[3][4] = {
            {1.0f, 0.0f, 0.0f, g_game_state.player_x},
            {0.0f, 1.0f, 0.0f, g_game_state.player_y},
            {0.0f, 0.0f, 1.0f, 0.0f}
        };
        GXLoadPosMtxImm(view_matrix, 0);
        
        // Render game objects
        render_simple_quad();
        
        // End frame (wrapper handles buffer swap)
        AC_Vita_Frame_End();
        
        g_game_state.frame_count++;
        
        // Print stats every 60 frames
        if (g_game_state.frame_count % 60 == 0) {
            printf("📊 Frame %u, Player: (%.1f, %.1f), Buttons: 0x%04X\n",
                   g_game_state.frame_count, 
                   g_game_state.player_x, 
                   g_game_state.player_y,
                   g_game_state.current_buttons);
        }
    }
    
    printf("🎮 Demo finished after %u frames\n", g_game_state.frame_count);
}

int main() {
    printf("🚀 AC-Decomp Vita Wrapper Demo\n");
    printf("===============================\n");
    printf("This demo shows how AC-Decomp code runs on Vita\n");
    printf("with zero modifications thanks to our wrapper!\n\n");
    
    // Initialize everything automatically
    if (!AC_Vita_Platform_Init()) {
        printf("❌ Platform initialization failed\n");
        return -1;
    }
    
    // Register cleanup
    atexit(AC_Vita_Platform_Cleanup);
    
    // Load assets for this demo
    AC_Vita_Load_All_Assets_For_Scene("demo");
    
    // Run the game loop
    game_main_loop();
    
    printf("✅ Demo completed successfully!\n");
    return 0;
} 