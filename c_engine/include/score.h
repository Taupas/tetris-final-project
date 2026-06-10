#ifndef TETRIS_SCORE_H
#define TETRIS_SCORE_H

void score_add_lines(int *score, int *lines, int *level, int cleared);
int score_drop_points(int level);

#endif
