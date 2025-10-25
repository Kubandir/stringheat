#ifndef ENCODE_H
#define ENCODE_H

#include <stdint.h>
#include "audio.h"

char *normalize_text(const char *input);
uint32_t hash_seed(const char *seed);
AudioData *encode_text(const char *text, const char *seed);

#endif
