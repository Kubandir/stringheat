# stringheat

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
- **Binary Size:** ~260KB (stripped and UPX compressed, includes embedded soundfont)
- **Text Normalization:** Auto-converts to lowercase a-z and spaces (strips punctuation, numbers, diacritics)
- **Standalone:** Single binary, no runtime dependencies
  
## Clean

```bash
make clean      # Remove build artifacts
make distclean  # Remove dependencies too
```

## License

Public domain / MIT - use freely.
