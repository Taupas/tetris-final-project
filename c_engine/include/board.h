#ifndef TETRIS_BOARD_H
#define TETRIS_BOARD_H

#include "common.h"

/* 棋盤：二維陣列概念 + malloc 配置的動態格子記憶體 */
typedef struct {
    int *cells;
    int rows;
    int cols;
} Board;

Board *board_create(int rows, int cols);
void board_destroy(Board *b);
void board_init(Board *b);
void board_clear(Board *b);
int board_get(const Board *b, int row, int col);
void board_set(Board *b, int row, int col, int value);
int board_clear_lines(Board *b);

#endif
