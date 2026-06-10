#ifndef TETRIS_ENGINE_API_H
#define TETRIS_ENGINE_API_H

#include "common.h"

/* Flat state for ctypes: board[row*10+col], 0=empty 1-7=colors */
typedef struct {
    int board[BOARD_HEIGHT * BOARD_WIDTH];
    int active_rows[4];
    int active_cols[4];
    int active_count;
    int active_type;
    int next_type;
    int score;
    int lines;
    int level;
    int game_over;
} EngineState;

API_EXPORT int engine_create(unsigned int seed);
API_EXPORT void engine_destroy(int game_id);
API_EXPORT int engine_tick(int game_id);
API_EXPORT int engine_move_left(int game_id);
API_EXPORT int engine_move_right(int game_id);
API_EXPORT int engine_soft_drop(int game_id);
API_EXPORT int engine_rotate_cw(int game_id);
API_EXPORT int engine_rotate_ccw(int game_id);
API_EXPORT int engine_hard_drop(int game_id);
API_EXPORT int engine_get_state(int game_id, EngineState *out);

#endif
