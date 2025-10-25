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
    uint32_t char_hash = seed_hash;
    RNG rng;
    rng_init(&rng, seed_hash);

    int root_note = 48 + ((rng_next(&rng) % 12) * 2); // Stay in octave, even notes
    const int *scale = major_scale;
    int scale_choice = rng_next(&rng) % 4;
    if (scale_choice == 1)
        scale = minor_scale;
    else if (scale_choice == 2)
        scale = dorian_scale;
    else if (scale_choice == 3)
        scale = mixolydian_scale;

    int base_tempo_ms = 280 + (rng_next(&rng) % 120); // Slower, more relaxed
    int drum_pattern = rng_next(&rng) % 5;

    int melody_instrument = (rng_next(&rng) % 4);
    int melody_presets[] = {0, 24, 73, 11};
    int melody_preset = melody_presets[melody_instrument];

    int harmony_instrument = (rng_next(&rng) % 3);
    int harmony_presets[] = {0, 4, 48}; // Piano, EP, Strings
    int harmony_preset = harmony_presets[harmony_instrument];

    int bass_preset = 32 + (rng_next(&rng) % 4);
    int pad_preset = 88 + (rng_next(&rng) % 8);

    size_t text_len = strlen(text);
    size_t avg_char_duration_frames = (base_tempo_ms * 44100) / 1000;
    size_t total_frames = (text_len * avg_char_duration_frames) + (44100 * 2);

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

    size_t current_frame = 0;
    int beat_count = 0;
    int phrase_count = 0;
    int prev_melody_note = -1;
    int prev_harmony_note = -1;
    int prev_bass_note = -1;
    int prev_pad_notes[4] = {-1, -1, -1, -1};
    int last_scale_degree = 0; // Track for smoother melody

    for (size_t i = 0; i < text_len; i++)
    {
        char c = text[i];

        int duration_variation = ((i * 73 + char_hash) % 3);
        int duration_ms = base_tempo_ms;
        if (duration_variation == 0)
            duration_ms = duration_ms * 2 / 3;
        else if (duration_variation == 2)
            duration_ms = duration_ms * 4 / 3;

        size_t duration_frames = (duration_ms * 44100) / 1000;

        if (c == ' ')
        {
            size_t rest_frames = duration_frames / 3;
            if (current_frame + rest_frames > total_frames)
                rest_frames = total_frames - current_frame;
            if (rest_frames > 0)
            {
                audio_render_samples(audio->buffer + current_frame * 2, rest_frames);
                current_frame += rest_frames;
            }
            beat_count++;
            phrase_count++;
            continue;
        }

        int char_val = c - 'a';
        if (char_val < 0 || char_val > 25)
        {
            beat_count++;
            continue;
        }

        // Smoother melodic movement - prefer stepwise motion
        int scale_degree = char_val % 7;
        int degree_diff = abs(scale_degree - last_scale_degree);
        if (degree_diff > 3 && i > 0)
        {
            scale_degree = (last_scale_degree + (char_val % 3) - 1 + 7) % 7;
        }
        last_scale_degree = scale_degree;

        int chord_type = (char_val + i / 4) % 8; // Change chords less frequently

        // Gentler dynamics - much lower velocities
        float base_velocity = 0.25f + ((char_val % 8) * 0.015f);      // Reduced from 0.45
        float phrase_dynamics = 1.0f + ((phrase_count % 16) * 0.03f); // Gentler crescendo
        float velocity = base_velocity * phrase_dynamics;
        if (velocity > 0.5f)
            velocity = 0.5f; // Hard cap to prevent harshness

        int base_note = root_note + scale[scale_degree];
        int melody_note = base_note + 12;
        int bass_note = base_note - 12;

        // Release with slight overlap for smoothness
        if (prev_melody_note != -1 && prev_melody_note != melody_note)
        {
            audio_note_off(0, prev_melody_note);
        }
        if (prev_harmony_note != -1 && i % 2 == 0)
        {
            audio_note_off(1, prev_harmony_note);
        }

        // Melody - softer, more musical
        audio_note_on(0, melody_preset, melody_note, velocity * 0.6f);
        prev_melody_note = melody_note;

        // Harmony - play on downbeats and longer notes
        if (i % 2 == 0 || duration_variation == 2)
        {
            int harmony_note = base_note;
            audio_note_on(1, harmony_preset, harmony_note, velocity * 0.35f);
            prev_harmony_note = harmony_note;
        }

        // Bass - walking bass line, smoother transitions
        if (i % 2 == 0 || prev_bass_note == -1)
        {
            if (prev_bass_note != -1 && abs(bass_note - prev_bass_note) > 12)
            {
                bass_note = prev_bass_note + ((bass_note > prev_bass_note) ? 5 : -5);
            }
            if (prev_bass_note != -1)
                audio_note_off(2, prev_bass_note);
            audio_note_on(2, bass_preset, bass_note, velocity * 0.5f);
            prev_bass_note = bass_note;
        } // Pads - sustained chords every 4 beats, released after 3 beats
        if (i % 4 == 0)
        {
            for (int j = 0; j < 4; j++)
            {
                if (prev_pad_notes[j] != -1)
                    audio_note_off(3, prev_pad_notes[j]);
                prev_pad_notes[j] = -1;
            }

            for (int j = 0; j < 3 && chord_intervals[chord_type][j] != -1; j++)
            {
                int chord_note = base_note + chord_intervals[chord_type][j];
                audio_note_on(3, pad_preset, chord_note, velocity * 0.25f);
                prev_pad_notes[j] = chord_note;
            }
        }

        // Release pads after 3 beats for natural decay
        if (i % 4 == 3)
        {
            for (int j = 0; j < 4; j++)
            {
                if (prev_pad_notes[j] != -1)
                    audio_note_off(3, prev_pad_notes[j]);
                prev_pad_notes[j] = -1;
            }
        }

        // Gentler drums
        if (drum_patterns[drum_pattern][beat_count % 16])
        {
            audio_note_on(9, 0, 36, 0.4f);  // Softer kick
            audio_note_on(9, 0, 42, 0.25f); // Softer hi-hat
        }
        if (beat_count % 4 == 2)
        {
            audio_note_on(9, 0, 38, 0.35f); // Softer snare
        }
        if (beat_count % 16 == 0)
        {
            audio_note_on(9, 0, 49, 0.3f); // Occasional ride/crash
        }

        // Render in smaller chunks for smooth mixing
        size_t chunk_size = duration_frames / 4;
        for (int chunk = 0; chunk < 4; chunk++)
        {
            if (current_frame + chunk_size > total_frames)
                chunk_size = total_frames - current_frame;
            if (chunk_size > 0)
            {
                audio_render_samples(audio->buffer + current_frame * 2, chunk_size);
                current_frame += chunk_size;
            }
        }

        beat_count++;
        if (c == ' ' || i % 8 == 7)
            phrase_count++;
    }

    // Clean release of all notes
    if (prev_melody_note != -1)
        audio_note_off(0, prev_melody_note);
    if (prev_harmony_note != -1)
        audio_note_off(1, prev_harmony_note);
    if (prev_bass_note != -1)
        audio_note_off(2, prev_bass_note);
    for (int j = 0; j < 4; j++)
    {
        if (prev_pad_notes[j] != -1)
            audio_note_off(3, prev_pad_notes[j]);
    }

    // Render a short tail for note decay (500ms instead of 2 seconds)
    size_t tail_frames = 22050; // 0.5 seconds at 44100 Hz
    size_t tail_rendered = 0;
    while (tail_rendered < tail_frames && current_frame < total_frames)
    {
        size_t render = tail_frames - tail_rendered;
        if (render > 4410)
            render = 4410;
        if (current_frame + render > total_frames)
            render = total_frames - current_frame;
        if (render == 0)
            break;

        audio_render_samples(audio->buffer + current_frame * 2, render);
        current_frame += render;
        tail_rendered += render;
    }

    // Trim total frames to actual content (remove excess silence)
    audio->frame_count = current_frame;

    return audio;
}
