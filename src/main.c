#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "audio.h"
#include "encode.h"

static void print_usage(void)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  stringheat -s <seed> -e <text>       Encode text to WAV (stdout)\n");
    fprintf(stderr, "  stringheat -s <seed> -d <file>       Decode WAV file\n");
    fprintf(stderr, "  stringheat -r                        Generate random music (stdout)\n");
    exit(1);
}

static void generate_random_text(char *buffer, int length)
{
    const char *words[] = {
        "the", "quick", "brown", "fox", "jumps", "over", "lazy", "dog",
        "music", "sound", "wave", "rhythm", "melody", "harmony", "beat",
        "night", "day", "light", "dark", "sky", "ocean", "mountain", "river",
        "dream", "hope", "love", "peace", "time", "space", "echo", "song"};
    int word_count = sizeof(words) / sizeof(words[0]);
    int pos = 0;

    while (pos < length - 10)
    {
        const char *word = words[rand() % word_count];
        int word_len = strlen(word);

        if (pos + word_len + 1 >= length)
            break;

        strcpy(buffer + pos, word);
        pos += word_len;

        if (pos < length - 1)
        {
            buffer[pos++] = ' ';
        }
    }
    buffer[pos] = '\0';
}

int main(int argc, char **argv)
{
    char *seed = NULL;
    char *input_text = NULL;
    char *decode_file = NULL;
    int random_mode = 0;
    int opt;

    while ((opt = getopt(argc, argv, "s:e:d:r")) != -1)
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
        case 'r':
            random_mode = 1;
            break;
        default:
            print_usage();
        }
    }

    if (random_mode)
    {
        srand((unsigned int)time(NULL));

        int text_length = 25 + (rand() % 76); // 25 to 100 characters
        char *random_text = malloc(text_length + 1);
        if (!random_text)
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return 1;
        }

        generate_random_text(random_text, text_length);

        char random_seed[32];
        snprintf(random_seed, sizeof(random_seed), "%u", (unsigned int)rand());

        fprintf(stderr, "Random generation:\n");
        fprintf(stderr, "  Text: '%s'\n", random_text);
        fprintf(stderr, "  Length: %d chars\n", (int)strlen(random_text));
        fprintf(stderr, "  Seed: %s\n", random_seed);

        audio_init("soundfont.sf2");
        AudioData *audio = encode_text(random_text, random_seed);

        if (!audio)
        {
            fprintf(stderr, "Error: Encoding failed\n");
            audio_cleanup();
            free(random_text);
            return 1;
        }

        uint32_t seed_hash = hash_seed(random_seed);
        audio_write_wav(random_text, seed_hash, audio);

        fprintf(stderr, "Done\n");

        free(audio->buffer);
        free(audio);
        free(random_text);
        audio_cleanup();
        return 0;
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
