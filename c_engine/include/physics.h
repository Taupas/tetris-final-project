#ifndef TETRIS_PHYSICS_H
#define TETRIS_PHYSICS_H

#include "board.h"
#include "tetromino.h"

int physics_can_place(const Board *b, const Piece *p);
int physics_move(Board *b, Piece *p, int dx, int dy);
int physics_rotate(Board *b, Piece *p, int direction);
void physics_lock(Board *b, const Piece *p);
int physics_hard_drop(Board *b, Piece *p);

#endif
