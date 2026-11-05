CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c11 $(shell pkg-config --cflags sdl2 SDL2_ttf SDL2_mixer x11)
LDLIBS := $(shell pkg-config --libs sdl2 SDL2_ttf SDL2_mixer x11) -lm
SOURCES := src/main.c src/media.c src/extended/bank_hack.c src/extended/panam_extended.c src/extended/games_extended.c

all: wargame-c

wargame-c: $(SOURCES) src/media.h
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDLIBS)

clean:
	rm -f wargame-c

.PHONY: all clean
