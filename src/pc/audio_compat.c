/**
 * @file audio_compat.c
 * @brief Audio system compatibility layer
 *
 * Part of Layer 2 (Porting Abstraction Layer).
 * Provides audio playback using SDL2 audio.
 *
 * Note: Full audio implementation will require porting the
 * jaudio_NES sequencer system.
 */

#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "pc/platform.h"

/* Audio configuration */
#define AUDIO_SAMPLE_RATE   32000
#define AUDIO_CHANNELS      2
#define AUDIO_BUFFER_SIZE   1024

/* Audio state */
static SDL_AudioDeviceID g_audio_device = 0;
static int g_audio_initialized = 0;

/**
 * Audio callback (called by SDL)
 */
static void audio_callback(void* userdata, u8* stream, int len) {
    (void)userdata;

    /* Fill with silence for now */
    /* TODO: Read from audio buffer filled by jaudio_NES */
    memset(stream, 0, len);
}

/**
 * Initialize audio system
 */
void audio_init(void) {
    SDL_AudioSpec want, have;

    SDL_memset(&want, 0, sizeof(want));
    want.freq = AUDIO_SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = AUDIO_CHANNELS;
    want.samples = AUDIO_BUFFER_SIZE;
    want.callback = audio_callback;
    want.userdata = NULL;

    g_audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);

    if (g_audio_device == 0) {
        fprintf(stderr, "Failed to open audio device: %s\n", SDL_GetError());
        return;
    }

    printf("Audio initialized:\n");
    printf("  Sample rate: %d Hz\n", have.freq);
    printf("  Channels: %d\n", have.channels);
    printf("  Buffer size: %d samples\n", have.samples);

    /* Start audio playback */
    SDL_PauseAudioDevice(g_audio_device, 0);

    g_audio_initialized = 1;
}

/**
 * Shutdown audio system
 */
void audio_shutdown(void) {
    if (g_audio_device) {
        SDL_CloseAudioDevice(g_audio_device);
        g_audio_device = 0;
    }
    g_audio_initialized = 0;
}

/**
 * Check if audio is initialized
 */
int audio_is_initialized(void) {
    return g_audio_initialized;
}

/**
 * Submit audio samples to the output buffer
 * This will be called by the audio synthesis code
 */
void audio_submit_samples(s16* samples, int num_samples) {
    if (g_audio_device && samples && num_samples > 0) {
        SDL_QueueAudio(g_audio_device, samples, num_samples * sizeof(s16) * AUDIO_CHANNELS);
    }
}

/**
 * Get the number of queued audio samples
 */
int audio_get_queued_samples(void) {
    if (g_audio_device) {
        return SDL_GetQueuedAudioSize(g_audio_device) / (sizeof(s16) * AUDIO_CHANNELS);
    }
    return 0;
}

/* ============================================================================
 * N64 Audio Interface Stubs
 * These will be implemented properly when jaudio_NES is ported
 * ============================================================================ */

void osAiSetFrequency(u32 freq) {
    (void)freq;
    /* TODO: Adjust audio output frequency */
}

u32 osAiGetLength(void) {
    /* Return bytes remaining in DMA transfer */
    return 0;
}

s32 osAiSetNextBuffer(void* buf, u32 size) {
    (void)buf;
    (void)size;
    /* TODO: Queue audio buffer for playback */
    return 0;
}
