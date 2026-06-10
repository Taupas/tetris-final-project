#ifndef BLOCK_PUZZLE_API_H
#define BLOCK_PUZZLE_API_H

#include "common.h"

#define BLOCK_GRID 8
#define BLOCK_SLOTS 3
#define BLOCK_MAX_SHAPE 6
#define BLOCK_CELLS (BLOCK_GRID * BLOCK_GRID)
#define BLOCK_THEME_COUNT 8
#define BLOCK_SLOT_FLAT (BLOCK_SLOTS * BLOCK_MAX_SHAPE)

typedef struct {
    int board[BLOCK_CELLS];
    int slot_len[BLOCK_SLOTS];
    int slot_active[BLOCK_SLOTS];
    int slot_dr[BLOCK_SLOT_FLAT];
    int slot_dc[BLOCK_SLOT_FLAT];
    int score;
    int theme;
    int theme_changed;
    int game_over;
} BlockEngineState;

API_EXPORT int block_create(unsigned int seed);
API_EXPORT void block_destroy(int game_id);
API_EXPORT int block_place(int game_id, int slot, int row, int col);
API_EXPORT int block_get_state(int game_id, BlockEngineState *out);

#endif
