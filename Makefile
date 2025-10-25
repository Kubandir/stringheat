CC = gcc
CFLAGS = -O3 -flto -fdata-sections -ffunction-sections -fno-asynchronous-unwind-tables -fno-ident -fno-stack-protector -Wall -Isrc -Iinclude
LDFLAGS = -Wl,--gc-sections -Wl,--strip-all -Wl,--build-id=none -Wl,-z,norelro -static-libgcc -s -lm
TARGET = bin/stringheat
LIBS_OBJ = bin/libs.o
SRC = src/main.c src/audio.c src/encode.c
OBJ = bin/main.o bin/audio.o bin/encode.o
SOUNDFONT = bin/soundfont.sf2
SOUNDFONT_OBJ = bin/soundfont_data.o

all: deps $(TARGET)

$(TARGET): bin include $(LIBS_OBJ) $(SOUNDFONT_OBJ) $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS_OBJ) $(SOUNDFONT_OBJ) $(LDFLAGS)
	strip -R .comment -R .gnu.version --strip-unneeded $(TARGET)
	@if command -v upx >/dev/null 2>&1; then \
		upx --best --lzma $(TARGET) 2>/dev/null || upx -9 $(TARGET); \
		echo "UPX compression applied"; \
	else \
		echo "Warning: UPX not found, skipping compression (install with: sudo apt install upx)"; \
	fi

$(LIBS_OBJ): bin include/tsf.h src/lib_impl.c
	$(CC) $(CFLAGS) -c src/lib_impl.c -o $(LIBS_OBJ)

$(SOUNDFONT_OBJ): bin $(SOUNDFONT)
	xxd -i $(SOUNDFONT) | sed 's/unsigned char/const unsigned char/g; s/bin_soundfont_sf2/soundfont_sf2/g' > bin/soundfont_data.c
	$(CC) $(CFLAGS) -c bin/soundfont_data.c -o $(SOUNDFONT_OBJ)

bin/main.o: src/main.c src/audio.h src/encode.h
	$(CC) $(CFLAGS) -c src/main.c -o bin/main.o

bin/audio.o: src/audio.c src/audio.h include/tsf.h
	$(CC) $(CFLAGS) -c src/audio.c -o bin/audio.o

bin/encode.o: src/encode.c src/encode.h src/audio.h
	$(CC) $(CFLAGS) -c src/encode.c -o bin/encode.o

bin:
	mkdir -p bin

include:
	mkdir -p include

deps: include/tsf.h include/tml.h $(SOUNDFONT)

include/tsf.h:
	mkdir -p include
	curl -L -o include/tsf.h https://raw.githubusercontent.com/schellingb/TinySoundFont/master/tsf.h

include/tml.h:
	mkdir -p include
	curl -L -o include/tml.h https://raw.githubusercontent.com/schellingb/TinySoundFont/master/tml.h

$(SOUNDFONT):
	mkdir -p bin
	@echo "Downloading compact GM soundfont..."
	@curl -L -o $(SOUNDFONT) 'https://github.com/FluidSynth/fluidsynth/raw/master/sf2/VintageDreamsWaves-v2.sf2' || \
	curl -L -o $(SOUNDFONT) 'https://github.com/urish/cinto/raw/master/sounds/FluidR3_GM.sf2' || \
	(echo "Direct download failed, trying alternative..." && \
	 wget -O $(SOUNDFONT) 'https://sourceforge.net/projects/androidframe/files/soundfonts/TimGM6mb.sf2/download' || \
	 curl -L -o $(SOUNDFONT) 'https://archive.org/download/free-soundfonts-sf2-2019-04/FluidR3_GM.sf2')
	@echo "Verifying soundfont file..."
	@if file $(SOUNDFONT) | grep -q "HTML\|text"; then \
		echo "Error: Downloaded HTML instead of SF2 file. Trying alternative sources..."; \
		curl -L -o $(SOUNDFONT) 'https://ia802606.us.archive.org/7/items/free-soundfonts-sf2-2019-04/FluidR3_GM.sf2' || \
		curl -L -o $(SOUNDFONT) 'https://github.com/FluidSynth/fluidsynth/raw/v2.3.0/sf2/VintageDreamsWaves-v2.sf2'; \
	fi
	@ls -lh $(SOUNDFONT)
	@file $(SOUNDFONT)

clean:
	rm -rf bin/

distclean: clean
	rm -rf include/

.PHONY: all deps clean distclean
