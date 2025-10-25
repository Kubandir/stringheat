#include "audio.h"
#include "tsf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const unsigned char soundfont_sf2[];
extern const unsigned int soundfont_sf2_len;

static tsf *g_synth = NULL;

void audio_init(const char *soundfont_path)
{
    (void)soundfont_path;
    g_synth = tsf_load_memory(soundfont_sf2, soundfont_sf2_len);
    if (!g_synth)
    {
        fprintf(stderr, "Failed to load soundfont\n");
        exit(1);
    }
    tsf_set_output(g_synth, TSF_STEREO_INTERLEAVED, 44100, 0.0f);
}

void audio_cleanup(void)
{
    if (g_synth)
    {
        tsf_close(g_synth);
        g_synth = NULL;
    }
}

void audio_note_on(int channel, int preset, int note, float velocity)
{
    if (!g_synth)
        return;

    // For channel 9 (drums), always use bank 128 (drum kit)
    // For other channels, use bank 0 (melodic instruments)
    int is_drum = (channel == 9);
    tsf_channel_set_bank_preset(g_synth, channel, is_drum ? 128 : 0, preset);
    tsf_channel_note_on(g_synth, channel, note, velocity);
}

void audio_note_off(int channel, int note)
{
    if (!g_synth)
        return;
    tsf_channel_note_off(g_synth, channel, note);
}

void audio_render_samples(int16_t *buffer, size_t frames)
{
    if (!g_synth)
        return;
    tsf_render_short(g_synth, buffer, frames, 0);
}

int audio_write_wav(const char *text, uint32_t seed_hash, AudioData *data)
{
    size_t text_len = strlen(text);
    size_t meta_size = 8 + text_len;
    uint8_t *meta = malloc(meta_size);
    if (!meta)
        return 0;

    memcpy(meta, &seed_hash, 4);
    uint32_t len = (uint32_t)text_len;
    memcpy(meta + 4, &len, 4);

    for (size_t i = 0; i < text_len; i++)
    {
        meta[8 + i] = text[i] ^ ((seed_hash >> ((i % 4) * 8)) & 0xFF);
    }

    uint32_t data_size = (uint32_t)(data->frame_count * 4);
    uint32_t file_size = 36 + data_size + 8 + meta_size;

    FILE *out = fopen("/dev/stdout", "wb");
    if (!out)
    {
        free(meta);
        return 0;
    }

    fwrite("RIFF", 1, 4, out);
    fwrite(&file_size, 4, 1, out);
    fwrite("WAVE", 1, 4, out);

    fwrite("fmt ", 1, 4, out);
    uint32_t fmt_size = 16;
    fwrite(&fmt_size, 4, 1, out);
    uint16_t format_tag = 1;
    uint16_t channels = 2;
    uint32_t sample_rate = 44100;
    uint32_t byte_rate = 176400;
    uint16_t block_align = 4;
    uint16_t bits = 16;
    fwrite(&format_tag, 2, 1, out);
    fwrite(&channels, 2, 1, out);
    fwrite(&sample_rate, 4, 1, out);
    fwrite(&byte_rate, 4, 1, out);
    fwrite(&block_align, 2, 1, out);
    fwrite(&bits, 2, 1, out);

    fwrite("data", 1, 4, out);
    fwrite(&data_size, 4, 1, out);
    fwrite(data->buffer, 1, data_size, out);

    fwrite("shXX", 1, 4, out);
    fwrite(&meta_size, 4, 1, out);
    fwrite(meta, 1, meta_size, out);

    fclose(out);
    free(meta);
    return 1;
}

char *audio_read_metadata(const char *filename, uint32_t seed_hash)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *data = malloc(fsize);
    if (!data)
    {
        fclose(f);
        return NULL;
    }
    fread(data, 1, fsize, f);
    fclose(f);

    for (long i = 0; i < fsize - 16; i++)
    {
        if (data[i] == 's' && data[i + 1] == 'h' && data[i + 2] == 'X' && data[i + 3] == 'X')
        {
            uint32_t chunk_size;
            memcpy(&chunk_size, data + i + 4, 4);

            if (i + 8 + chunk_size > fsize)
                break;

            uint32_t stored_hash;
            memcpy(&stored_hash, data + i + 8, 4);

            if (stored_hash != seed_hash)
            {
                free(data);
                return NULL;
            }

            uint32_t text_len;
            memcpy(&text_len, data + i + 12, 4);

            if (text_len > 10000 || i + 16 + text_len > fsize)
                break;

            char *text = malloc(text_len + 1);
            if (!text)
            {
                free(data);
                return NULL;
            }

            for (uint32_t j = 0; j < text_len; j++)
            {
                text[j] = data[i + 16 + j] ^ ((seed_hash >> ((j % 4) * 8)) & 0xFF);
            }
            text[text_len] = '\0';

            free(data);
            return text;
        }
    }

    free(data);
    return NULL;
}
