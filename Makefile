CC = gcc
CFLAGS = -O2 -Wall -I c_engine/include
SRC = c_engine/src/queue.c c_engine/src/board.c c_engine/src/tetromino.c c_engine/src/physics.c \
      c_engine/src/score.c c_engine/src/engine_api.c c_engine/src/block_puzzle_api.c

.PHONY: all clean

all:
	mkdir -p lib
	$(CC) -shared $(CFLAGS) $(SRC) -o lib/tetris_engine.dll 2>/dev/null || \
	$(CC) -shared -fPIC $(CFLAGS) $(SRC) -o lib/libtetris_engine.so

clean:
	rm -f lib/tetris_engine.dll lib/libtetris_engine.so
