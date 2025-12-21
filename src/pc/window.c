/**
 * @file window.c
 * @brief SDL window and OpenGL context management
 *
 * Part of Layer 3 (Host System Implementation).
 * Handles window creation, OpenGL context, and event processing.
 */

#include <stdio.h>
#include <SDL2/SDL.h>
#include <GL/gl.h>

#include "pc/platform.h"

/* Window state */
static SDL_Window* g_window = NULL;
static SDL_GLContext g_gl_context = NULL;
static int g_should_close = 0;
static int g_window_width = SCREEN_WIDTH;
static int g_window_height = SCREEN_HEIGHT;

/* Input state for player movement */
static int g_key_up = 0;
static int g_key_down = 0;
static int g_key_left = 0;
static int g_key_right = 0;

/**
 * Initialize window and OpenGL context
 */
void window_init(int width, int height, const char* title) {
    g_window_width = width;
    g_window_height = height;

    /* Set OpenGL attributes */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    /* Create window */
    g_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!g_window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        return;
    }

    /* Create OpenGL context */
    g_gl_context = SDL_GL_CreateContext(g_window);
    if (!g_gl_context) {
        fprintf(stderr, "Failed to create OpenGL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_window);
        g_window = NULL;
        return;
    }

    /* Enable VSync */
    SDL_GL_SetSwapInterval(1);

    /* Set up OpenGL viewport */
    glViewport(0, 0, width, height);

    /* Enable depth testing */
    glEnable(GL_DEPTH_TEST);

    /* Set up projection matrix (basic orthographic for now) */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1000, 1000);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    printf("Window created: %dx%d\n", width, height);
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
}

/**
 * Shutdown window
 */
void window_shutdown(void) {
    if (g_gl_context) {
        SDL_GL_DeleteContext(g_gl_context);
        g_gl_context = NULL;
    }

    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }
}

/**
 * Swap front and back buffers
 */
void window_swap_buffers(void) {
    if (g_window) {
        SDL_GL_SwapWindow(g_window);
    }
}

/**
 * Check if window should close
 */
int window_should_close(void) {
    return g_should_close;
}

/**
 * Poll and process window events
 */
void window_poll_events(void) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                g_should_close = 1;
                break;

            case SDL_KEYDOWN:
                /* ESC to quit */
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    g_should_close = 1;
                }
                /* Movement keys */
                if (event.key.keysym.sym == SDLK_w || event.key.keysym.sym == SDLK_UP) {
                    g_key_up = 1;
                    printf("[INPUT] W/UP pressed\n"); fflush(stdout);
                }
                if (event.key.keysym.sym == SDLK_s || event.key.keysym.sym == SDLK_DOWN) {
                    g_key_down = 1;
                    printf("[INPUT] S/DOWN pressed\n"); fflush(stdout);
                }
                if (event.key.keysym.sym == SDLK_a || event.key.keysym.sym == SDLK_LEFT) {
                    g_key_left = 1;
                    printf("[INPUT] A/LEFT pressed\n"); fflush(stdout);
                }
                if (event.key.keysym.sym == SDLK_d || event.key.keysym.sym == SDLK_RIGHT) {
                    g_key_right = 1;
                    printf("[INPUT] D/RIGHT pressed\n"); fflush(stdout);
                }
                break;

            case SDL_KEYUP:
                /* Movement keys release */
                if (event.key.keysym.sym == SDLK_w || event.key.keysym.sym == SDLK_UP) {
                    g_key_up = 0;
                }
                if (event.key.keysym.sym == SDLK_s || event.key.keysym.sym == SDLK_DOWN) {
                    g_key_down = 0;
                }
                if (event.key.keysym.sym == SDLK_a || event.key.keysym.sym == SDLK_LEFT) {
                    g_key_left = 0;
                }
                if (event.key.keysym.sym == SDLK_d || event.key.keysym.sym == SDLK_RIGHT) {
                    g_key_right = 0;
                }
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    g_window_width = event.window.data1;
                    g_window_height = event.window.data2;
                    glViewport(0, 0, g_window_width, g_window_height);
                    printf("Window resized: %dx%d\n", g_window_width, g_window_height);
                }
                break;
        }
    }
}

/**
 * Get window dimensions
 */
void window_get_size(int* width, int* height) {
    if (width) *width = g_window_width;
    if (height) *height = g_window_height;
}

/**
 * Get SDL window handle (for input system)
 */
SDL_Window* window_get_sdl_window(void) {
    return g_window;
}

/**
 * Get movement input state
 * Returns movement direction: -1 to 1 for each axis
 */
void input_get_movement(float* dx, float* dz) {
    *dx = 0.0f;
    *dz = 0.0f;

    if (g_key_left) *dx -= 1.0f;
    if (g_key_right) *dx += 1.0f;
    if (g_key_up) *dz -= 1.0f;    /* Up = forward = -Z in world */
    if (g_key_down) *dz += 1.0f;  /* Down = back = +Z in world */
}
