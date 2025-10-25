CC = gcc
CFLAGS = -O3 -flto -fdata-sections -ffunction-sections -fno-asynchronous-unwind-tables -fno-ident -fno-stack-protector -Wall -Isrc
LDFLAGS = -Wl,--gc-sections -Wl,--strip-all -Wl,--build-id=none -Wl,-z,norelro -static-libgcc -s -lm
TARGET = bin/stringheat
LIBS_OBJ = bin/libs.o
SRC = src/main.c src/audio.c src/encode.c
OBJ = $(SRC:.c=.o)
SOUNDFONT_OBJ = bin/soundfont_data.o

all: deps $(TARGET)

$(TARGET): bin $(LIBS_OBJ) $(SOUNDFONT_OBJ) $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) src/main.o src/audio.o src/encode.o $(LIBS_OBJ) $(SOUNDFONT_OBJ) $(LDFLAGS)
	strip -R .comment -R .gnu.version --strip-unneeded $(TARGET)
	@if command -v upx >/dev/null 2>&1; then \
		upx --best --lzma $(TARGET) 2>/dev/null || upx -9 $(TARGET); \
		echo "UPX compression applied"; \
	else \
		echo "Warning: UPX not found, skipping compression (install with: sudo apt install upx)"; \
	fi

$(LIBS_OBJ): bin src/lib_impl.c src/tsf.h
	$(CC) $(CFLAGS) -c src/lib_impl.c -o $(LIBS_OBJ)

$(SOUNDFONT_OBJ): bin soundfont.sf2
	xxd -i soundfont.sf2 | sed 's/unsigned char/const unsigned char/g' > bin/soundfont_data.c
	$(CC) $(CFLAGS) -c bin/soundfont_data.c -o $(SOUNDFONT_OBJ)

bin:
	mkdir -p bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

deps: src/tsf.h src/tml.h soundfont.sf2

src/tsf.h:
	curl -L -o src/tsf.h https://raw.githubusercontent.com/schellingb/TinySoundFont/master/tsf.h

src/tml.h:
	curl -L -o src/tml.h https://raw.githubusercontent.com/schellingb/TinySoundFont/master/tml.h

soundfont.sf2:
	curl -L -o soundfont.sf2 https://github.com/FluidSynth/fluidsynth/raw/master/sf2/VintageDreamsWaves-v2.sf2

clean:
	rm -rf bin/ src/*.o

distclean: clean
	rm -f src/tsf.h src/tml.h soundfont.sf2

.PHONY: all deps clean distclean
