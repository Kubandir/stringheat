#include "encode.h"
#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

typedef struct
{
    uint32_t state;
} RNG;

static void rng_init(RNG *rng, uint32_t seed)
{
    rng->state = seed;
}

static uint32_t rng_next(RNG *rng)
{
    rng->state = rng->state * 1664525 + 1013904223;
    return rng->state;
}

char *normalize_text(const char *input)
{
    size_t len = strlen(input);
    char *output = malloc(len + 1);
    if (!output)
        return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++)
    {
        char c = input[i];
        if (c >= 'A' && c <= 'Z')
        {
            output[j++] = c + 32;
        }
        else if ((c >= 'a' && c <= 'z') || c == ' ')
        {
            output[j++] = c;
        }
    }
    output[j] = '\0';
    return output;
}

uint32_t hash_seed(const char *seed)
{
    uint32_t hash = 5381;
    while (*seed)
    {
        hash = ((hash << 5) + hash) + (*seed++);
    }
    return hash;
}

static const int major_scale[] = {0, 2, 4, 5, 7, 9, 11};
static const int minor_scale[] = {0, 2, 3, 5, 7, 8, 10};
static const int dorian_scale[] = {0, 2, 3, 5, 7, 9, 10};
static const int mixolydian_scale[] = {0, 2, 4, 5, 7, 9, 10};

static const int chord_intervals[][4] = {
    {0, 4, 7, -1},
    {0, 3, 7, -1},
    {0, 4, 7, 10},
    {0, 3, 7, 10},
    {0, 5, 7, -1},
    {0, 3, 6, -1},
    {0, 4, 8, -1},
    {0, 4, 7, 11}};

static const int drum_patterns[][16] = {
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
    {1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0},
    {1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0},
    {1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1},
    {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0}};

AudioData *encode_text(const char *text, const char *seed)
{
    uint32_t seed_hash = hash_seed(seed);
    RNG rng;
    rng_init(&rng, seed_hash);

    int root_note = 48 + (rng_next(&rng) % 24);
    const int *scale = major_scale;
    int scale_choice = rng_next(&rng) % 4;
    if (scale_choice == 1)
        scale = minor_scale;
    else if (scale_choice == 2)
        scale = dorian_scale;
    else if (scale_choice == 3)
        scale = mixolydian_scale;

    int tempo_ms = 300 + (rng_next(&rng) % 200);
    int drum_pattern = rng_next(&rng) % 5;

    size_t text_len = strlen(text);
    size_t char_duration_frames = (tempo_ms * 44100) / 1000;
    size_t total_frames = (text_len * char_duration_frames) + (44100 * 2);

    AudioData *audio = malloc(sizeof(AudioData));
    if (!audio)
        return NULL;

    audio->buffer = calloc(total_frames * 2, sizeof(int16_t));
    audio->frame_count = total_frames;
    audio->sample_rate = 44100;

    if (!audio->buffer)
    {
        free(audio);
        return NULL;
    }

    int piano_preset = 0;
    int guitar_preset = (rng_next(&rng) % 2) ? 24 : 27;
    int bass_preset = (rng_next(&rng) % 2) ? 32 : 38;
    int pad_preset = 48 + (rng_next(&rng) % 16);
    int use_choir = rng_next(&rng) % 3 == 0;

    size_t current_frame = 0;
    int beat_count = 0;

    for (size_t i = 0; i < text_len; i++)
    {
        char c = text[i];
        int duration_ms = tempo_ms + ((i * 37) % 100);
        size_t duration_frames = (duration_ms * 44100) / 1000;

        if (c == ' ')
        {
            size_t rest_frames = duration_frames / 2;
            if (current_frame + rest_frames > total_frames)
                rest_frames = total_frames - current_frame;
            if (rest_frames > 0)
            {
                audio_render_samples(audio->buffer + current_frame * 2, rest_frames);
                current_frame += rest_frames;
            }
            beat_count++;
            continue;
        }

        int char_val = c - 'a';
        if (char_val < 0 || char_val > 25)
        {
            if (current_frame + duration_frames > total_frames)
                duration_frames = total_frames - current_frame;
            if (duration_frames > 0)
            {
                audio_render_samples(audio->buffer + current_frame * 2, duration_frames);
                current_frame += duration_frames;
            }
            beat_count++;
            continue;
        }

        int scale_degree = char_val % 7;
        int chord_type = (char_val + i) % 8;
        float velocity = 0.35f + ((char_val % 10) * 0.02f);

        int base_note = root_note + scale[scale_degree];

        for (int j = 0; j < 4 && chord_intervals[chord_type][j] != -1; j++)
        {
            int note = base_note + chord_intervals[chord_type][j];
            audio_note_on(0, piano_preset, note, velocity * 0.7f);
            if (j < 3)
            {
                audio_note_on(3, pad_preset, note - 12, velocity * 0.3f);
            }
        }

        audio_note_on(1, guitar_preset, base_note + 12, velocity * 0.5f);
        audio_note_on(2, bass_preset, base_note - 24, velocity * 0.6f);

        if (use_choir && i % 4 == 0)
        {
            audio_note_on(4, 52, base_note, velocity * 0.4f);
        }

        if (drum_patterns[drum_pattern][beat_count % 16])
        {
            audio_note_on(9, 0, 36, 0.6f);
            audio_note_on(9, 0, 42, 0.4f);
        }
        if (beat_count % 4 == 2)
        {
            audio_note_on(9, 0, 38, 0.5f);
        }
        if (beat_count % 8 == 0)
        {
            audio_note_on(9, 0, 49, 0.45f);
        }

        size_t render_frames = duration_frames;
        if (current_frame + render_frames > total_frames)
            render_frames = total_frames - current_frame;

        if (render_frames > 0)
        {
            audio_render_samples(audio->buffer + current_frame * 2, render_frames);
            current_frame += render_frames;
        }

        for (int j = 0; j < 4 && chord_intervals[chord_type][j] != -1; j++)
        {
            int note = base_note + chord_intervals[chord_type][j];
            audio_note_off(0, note);
            if (j < 3)
            {
                audio_note_off(3, note - 12);
            }
        }
        audio_note_off(1, base_note + 12);
        audio_note_off(2, base_note - 24);
        if (use_choir && i % 4 == 0)
        {
            audio_note_off(4, base_note);
        }

        beat_count++;
    }

    while (current_frame < total_frames)
    {
        size_t remain = total_frames - current_frame;
        size_t render = remain > 4410 ? 4410 : remain;
        audio_render_samples(audio->buffer + current_frame * 2, render);
        current_frame += render;
    }

    return audio;
}
