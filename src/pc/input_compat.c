/**
 * @file input_compat.c
 * @brief N64 controller input compatibility layer
 *
 * Part of Layer 2 (Porting Abstraction Layer).
 * Maps SDL gamepad/keyboard input to N64 controller format.
 */

#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "pc/os_compat.h"

/* Controller state */
static SDL_GameController* g_controllers[MAXCONTROLLERS] = {0};
static OSContPad g_pad_data[MAXCONTROLLERS] = {0};
static OSContStatus g_cont_status[MAXCONTROLLERS] = {0};
static u8 g_controller_bitpattern = 0;

/* Keyboard state for player 1 fallback */
static const u8* g_keyboard_state = NULL;

/**
 * Initialize input system
 */
void input_init(void) {
    /* Initialize controller subsystem */
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);

    /* Get keyboard state */
    g_keyboard_state = SDL_GetKeyboardState(NULL);

    /* Open any connected controllers */
    int num_joysticks = SDL_NumJoysticks();
    printf("Found %d joysticks\n", num_joysticks);

    int controller_index = 0;
    for (int i = 0; i < num_joysticks && controller_index < MAXCONTROLLERS; i++) {
        if (SDL_IsGameController(i)) {
            g_controllers[controller_index] = SDL_GameControllerOpen(i);
            if (g_controllers[controller_index]) {
                const char* name = SDL_GameControllerName(g_controllers[controller_index]);
                printf("Controller %d: %s\n", controller_index, name ? name : "Unknown");
                g_controller_bitpattern |= (1 << controller_index);
                g_cont_status[controller_index].type = 0x0500; /* Standard controller */
                g_cont_status[controller_index].status = 0;
                controller_index++;
            }
        }
    }

    /* Always report at least one controller (keyboard fallback) */
    if (controller_index == 0) {
        g_controller_bitpattern = 0x01;
        g_cont_status[0].type = 0x0500;
        g_cont_status[0].status = 0;
        printf("No controllers found, using keyboard for player 1\n");
    }
}

/**
 * Update input state (call once per frame)
 */
void input_update(void) {
    /* SDL_PollEvent is called by window_poll_events */
    /* Just update gamepad state here */

    for (int i = 0; i < MAXCONTROLLERS; i++) {
        g_pad_data[i].button = 0;
        g_pad_data[i].stick_x = 0;
        g_pad_data[i].stick_y = 0;
        g_pad_data[i].errno = 0;

        if (g_controllers[i]) {
            /* Read gamepad buttons */
            if (SDL_GameControllerGetButton(g_controllers[i], SDL_CONTROLLER_BUTTON_A))
                g_pad_data[i].button |= CONT_A;
            if (SDL_GameControllerGetButton(g_controllers[i], SDL_CONTROLLER_BUTTON_B))
                g_pad_data[i].button |= CONT_B;
            if (SDL_GameControllerGetButton(g_controllers[i], SDL_CONTROLLER_BUTTON_X))
                g_pad_data[i].button |= CONT_C_LEFT;  /* Map X to C-Left */
            if (SDL_GameControllerGetButton(g_controllers[i], SDL_CONTROLLER_BUTTON_Y))
                g_pad_data[i].button |= CONT_C_UP;    /* Map Y to C-Up */
            if (SDL_GameControllerGetButton(g_controllers[i], SDL_CONTROLLER_BUTTON_START))
                g_pad_data[i].button |= CONT_START;
            if (SDL_GameControllerGetButton(g_controllers[i], SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
                g_pad_data[i].button |= CONT_L;
            if (SDL_GameControllerGetButton(g_controllers[i], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
                g_pad_data[i].button |= CONT_R;
            if (SDL_GameControllerGetButton(g_controllers[i], SDL_CONTROLLER_BUTTON_DPAD_UP))
                g_pad_data[i].button |= CONT_UP;
            if (SDL_GameControllerGetButton(g_controllers[i], SDL_CONTROLLER_BUTTON_DPAD_DOWN))
                g_pad_data[i].button |= CONT_DOWN;
            if (SDL_GameControllerGetButton(g_controllers[i], SDL_CONTROLLER_BUTTON_DPAD_LEFT))
                g_pad_data[i].button |= CONT_LEFT;
            if (SDL_GameControllerGetButton(g_controllers[i], SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
                g_pad_data[i].button |= CONT_RIGHT;

            /* Read analog stick */
            s16 lx = SDL_GameControllerGetAxis(g_controllers[i], SDL_CONTROLLER_AXIS_LEFTX);
            s16 ly = SDL_GameControllerGetAxis(g_controllers[i], SDL_CONTROLLER_AXIS_LEFTY);

            /* Scale from SDL range (-32768 to 32767) to N64 range (-80 to 80) */
            g_pad_data[i].stick_x = (s8)(lx / 410);
            g_pad_data[i].stick_y = (s8)(-ly / 410); /* Invert Y */

            /* Apply deadzone */
            if (g_pad_data[i].stick_x > -8 && g_pad_data[i].stick_x < 8)
                g_pad_data[i].stick_x = 0;
            if (g_pad_data[i].stick_y > -8 && g_pad_data[i].stick_y < 8)
                g_pad_data[i].stick_y = 0;

            /* Right stick for C buttons */
            s16 rx = SDL_GameControllerGetAxis(g_controllers[i], SDL_CONTROLLER_AXIS_RIGHTX);
            s16 ry = SDL_GameControllerGetAxis(g_controllers[i], SDL_CONTROLLER_AXIS_RIGHTY);
            if (rx < -16000) g_pad_data[i].button |= CONT_C_LEFT;
            if (rx > 16000) g_pad_data[i].button |= CONT_C_RIGHT;
            if (ry < -16000) g_pad_data[i].button |= CONT_C_UP;
            if (ry > 16000) g_pad_data[i].button |= CONT_C_DOWN;
        }
    }

    /* Keyboard fallback for player 1 */
    if (g_keyboard_state) {
        /* WASD for analog stick */
        if (g_keyboard_state[SDL_SCANCODE_W]) g_pad_data[0].stick_y = 80;
        if (g_keyboard_state[SDL_SCANCODE_S]) g_pad_data[0].stick_y = -80;
        if (g_keyboard_state[SDL_SCANCODE_A]) g_pad_data[0].stick_x = -80;
        if (g_keyboard_state[SDL_SCANCODE_D]) g_pad_data[0].stick_x = 80;

        /* Arrow keys for D-Pad */
        if (g_keyboard_state[SDL_SCANCODE_UP]) g_pad_data[0].button |= CONT_UP;
        if (g_keyboard_state[SDL_SCANCODE_DOWN]) g_pad_data[0].button |= CONT_DOWN;
        if (g_keyboard_state[SDL_SCANCODE_LEFT]) g_pad_data[0].button |= CONT_LEFT;
        if (g_keyboard_state[SDL_SCANCODE_RIGHT]) g_pad_data[0].button |= CONT_RIGHT;

        /* Other buttons */
        if (g_keyboard_state[SDL_SCANCODE_Z]) g_pad_data[0].button |= CONT_A;
        if (g_keyboard_state[SDL_SCANCODE_X]) g_pad_data[0].button |= CONT_B;
        if (g_keyboard_state[SDL_SCANCODE_RETURN]) g_pad_data[0].button |= CONT_START;
        if (g_keyboard_state[SDL_SCANCODE_Q]) g_pad_data[0].button |= CONT_L;
        if (g_keyboard_state[SDL_SCANCODE_E]) g_pad_data[0].button |= CONT_R;

        /* IJKL for C buttons */
        if (g_keyboard_state[SDL_SCANCODE_I]) g_pad_data[0].button |= CONT_C_UP;
        if (g_keyboard_state[SDL_SCANCODE_K]) g_pad_data[0].button |= CONT_C_DOWN;
        if (g_keyboard_state[SDL_SCANCODE_J]) g_pad_data[0].button |= CONT_C_LEFT;
        if (g_keyboard_state[SDL_SCANCODE_L]) g_pad_data[0].button |= CONT_C_RIGHT;
    }
}

/**
 * Shutdown input system
 */
void input_shutdown(void) {
    for (int i = 0; i < MAXCONTROLLERS; i++) {
        if (g_controllers[i]) {
            SDL_GameControllerClose(g_controllers[i]);
            g_controllers[i] = NULL;
        }
    }
}

/* ============================================================================
 * N64 Controller API Implementation
 * ============================================================================ */

s32 osContInit(OSMesgQueue* mq, u8* bitpattern, OSContStatus* status) {
    (void)mq;

    if (bitpattern) {
        *bitpattern = g_controller_bitpattern;
    }

    if (status) {
        memcpy(status, g_cont_status, sizeof(OSContStatus) * MAXCONTROLLERS);
    }

    return 0;
}

s32 osContStartQuery(OSMesgQueue* mq) {
    (void)mq;
    return 0;
}

s32 osContStartReadData(OSMesgQueue* mq) {
    (void)mq;
    return 0;
}

s32 osContGetQuery(OSContStatus* status) {
    if (status) {
        memcpy(status, g_cont_status, sizeof(OSContStatus) * MAXCONTROLLERS);
    }
    return 0;
}

void osContGetReadData(OSContPad* pad) {
    if (pad) {
        memcpy(pad, g_pad_data, sizeof(OSContPad) * MAXCONTROLLERS);
    }
}
