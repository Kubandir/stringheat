# stringheat

A minimal, high-performance audio steganography CLI tool that encodes text strings into musical compositions using deterministic patterns.

## Features

- **Encode text into pleasant-sounding musical tracks** - Uses real music theory with chord progressions, harmonies, and scales
- **Decode audio files back to original strings** - Metadata embedded in WAV file survives standard audio operations
- **Deterministic generation** - Same seed + text = identical audio every time
- **Multi-instrument layering** - Piano, bass, guitar, pads, choir, and drums
- **Standalone binary** - Soundfont embedded, no external dependencies needed
- **Optimized C implementation** - Heavily stripped and UPX compressed
- **Secure** - Encrypted metadata protected by seed-based hash

## Build

```bash
make deps    # Download dependencies (TinySoundFont, soundfont)
make         # Build optimized binary
```

## Usage

**Encode text to WAV:**
```bash
./bin/stringheat -s "myseed" -e "hello world" > output.wav
```

**Decode WAV file:**
```bash
./bin/stringheat -s "myseed" -d output.wav
```

## Examples

```bash
# Encode a secret message
./bin/stringheat -s "secretkey" -e "The treasure is buried at midnight" > treasure.wav

# Decode the message
./bin/stringheat -s "secretkey" -d treasure.wav
# Output: Decoded: 'the treasure is buried at midnight'

# Wrong seed fails
./bin/stringheat -s "wrongkey" -d treasure.wav
# Output: Error: Decoding failed (wrong seed or corrupted file)
```

## Technical Details

- **Language:** C
- **Dependencies:** TinySoundFont (TSF) for synthesis
- **Output:** 44.1kHz 16-bit stereo WAV
- **Encoding:** Custom RIFF chunk with XOR-encrypted metadata
- **Binary Size:** ~1.5MB (stripped and UPX compressed, includes embedded soundfont)
- **Text Normalization:** Auto-converts to lowercase a-z and spaces (strips punctuation, numbers, diacritics)
- **Standalone:** Single binary, no runtime dependencies

## Music Theory Implementation

- Proven chord progressions (I-IV-V-I, ii-V-I, vi-IV-I-V)
- Diatonic scales (Major, Minor, Dorian, Mixolydian)
- Chord types: major, minor, 7th, sus4, dim, aug
- Multi-instrument: piano, bass, guitar, pads, optional choir
- Rhythm section with 5 different drum patterns
- Proper voice leading and note overlapping for smooth transitions

## Clean

```bash
make clean      # Remove build artifacts
make distclean  # Remove dependencies too
```

## License

Public domain / MIT - use freely.
