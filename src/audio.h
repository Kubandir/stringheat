#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stddef.h>

typedef struct
{
    int16_t *buffer;
    size_t frame_count;
    int sample_rate;
} AudioData;

void audio_init(const char *soundfont_path);
void audio_cleanup(void);
void audio_note_on(int channel, int preset, int note, float velocity);
void audio_note_off(int channel, int note);
void audio_render_samples(int16_t *buffer, size_t frames);
int audio_write_wav(const char *text, uint32_t seed_hash, AudioData *data);
char *audio_read_metadata(const char *filename, uint32_t seed_hash);

#endif
