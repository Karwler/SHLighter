CC = gcc
CFLAGS = -std=c11

all: release

debug: CFLAGS += -g
debug: SHLighter

release: CFLAGS += -O3 -s
release: SHLighter

SHLighter: src/shlighter_sender.c
	mkdir -p build
	${CC} ${CFLAGS} -o build/shlighter_sender src/shlighter_sender.c -lSDL2 -lSDL2_net
	cp src/shlighter_receiver.py build
	chmod +x build/shlighter_receiver.py
