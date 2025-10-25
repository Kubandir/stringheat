#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "audio.h"
#include "encode.h"

static void print_usage(void)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  stringheat -s <seed> -e <text>       Encode text to WAV (stdout)\n");
    fprintf(stderr, "  stringheat -s <seed> -d <file>       Decode WAV file\n");
    exit(1);
}

int main(int argc, char **argv)
{
    char *seed = NULL;
    char *input_text = NULL;
    char *decode_file = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "s:e:d:")) != -1)
    {
        switch (opt)
        {
        case 's':
            seed = optarg;
            break;
        case 'e':
            input_text = optarg;
            break;
        case 'd':
            decode_file = optarg;
            break;
        default:
            print_usage();
        }
    }

    if (!seed)
    {
        fprintf(stderr, "Error: Seed required\n");
        print_usage();
    }

    if (input_text && decode_file)
    {
        fprintf(stderr, "Error: Cannot encode and decode simultaneously\n");
        print_usage();
    }

    if (!input_text && !decode_file)
    {
        fprintf(stderr, "Error: Specify -e or -d\n");
        print_usage();
    }

    if (input_text)
    {
        char *normalized = normalize_text(input_text);
        if (!normalized)
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return 1;
        }

        fprintf(stderr, "Encoding: '%s' (%zu chars)\n", normalized, strlen(normalized));

        audio_init("soundfont.sf2");
        AudioData *audio = encode_text(normalized, seed);

        if (!audio)
        {
            fprintf(stderr, "Error: Encoding failed\n");
            audio_cleanup();
            free(normalized);
            return 1;
        }

        uint32_t seed_hash = hash_seed(seed);
        audio_write_wav(normalized, seed_hash, audio);

        fprintf(stderr, "Done\n");

        free(audio->buffer);
        free(audio);
        free(normalized);
        audio_cleanup();
    }
    else if (decode_file)
    {
        uint32_t seed_hash = hash_seed(seed);
        char *decoded = audio_read_metadata(decode_file, seed_hash);

        if (!decoded)
        {
            fprintf(stderr, "Error: Decoding failed (wrong seed or corrupted file)\n");
            return 1;
        }

        printf("Decoded: '%s'\n", decoded);
        free(decoded);
    }

    return 0;
}
