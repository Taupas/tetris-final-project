#ifndef TETRIS_TETROMINO_H
#define TETRIS_TETROMINO_H

#include "common.h"

typedef struct {
    int type;
    int rotation;
    int x;
    int y;
} Piece;

void piece_get_cells(int type, int rotation, int x, int y, int out_rows[4], int out_cols[4]);
int piece_type_color(int type);

#endif
