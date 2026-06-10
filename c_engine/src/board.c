#include "../include/board.h"
#include <stdlib.h>
#include <string.h>

Board *board_create(int rows, int cols) {
    Board *b;
    size_t n;
    if (rows <= 0 || cols <= 0) {
        return NULL;
    }
    b = (Board *)malloc(sizeof(Board));
    if (!b) {
        return NULL;
    }
    n = (size_t)rows * (size_t)cols;
    b->cells = (int *)malloc(n * sizeof(int));
    if (!b->cells) {
        free(b);
        return NULL;
    }
    b->rows = rows;
    b->cols = cols;
    board_clear(b);
    return b;
}

void board_destroy(Board *b) {
    if (!b) {
        return;
    }
    free(b->cells);
    free(b);
}

void board_init(Board *b) {
    if (b) {
        board_clear(b);
    }
}

void board_clear(Board *b) {
    size_t n;
    if (!b || !b->cells) {
        return;
    }
    n = (size_t)b->rows * (size_t)b->cols;
    memset(b->cells, 0, n * sizeof(int));
}

static int idx(const Board *b, int row, int col) {
    return row * b->cols + col;
}

int board_get(const Board *b, int row, int col) {
    if (!b || !b->cells || row < 0 || row >= b->rows || col < 0 || col >= b->cols) {
        return -1;
    }
    return b->cells[idx(b, row, col)];
}

void board_set(Board *b, int row, int col, int value) {
    if (!b || !b->cells || row < 0 || row >= b->rows || col < 0 || col >= b->cols) {
        return;
    }
    b->cells[idx(b, row, col)] = value;
}

int board_clear_lines(Board *b) {
    int cleared = 0;
    int r = b->rows - 1;

    if (!b || !b->cells) {
        return 0;
    }

    while (r >= 0) {
        int c;
        int full = 1;
        for (c = 0; c < b->cols; c++) {
            if (b->cells[idx(b, r, c)] == 0) {
                full = 0;
                break;
            }
        }
        if (!full) {
            r--;
            continue;
        }
        cleared++;
        {
            int rr;
            for (rr = r; rr > 0; rr--) {
                for (c = 0; c < b->cols; c++) {
                    b->cells[idx(b, rr, c)] = b->cells[idx(b, rr - 1, c)];
                }
            }
            for (c = 0; c < b->cols; c++) {
                b->cells[idx(b, 0, c)] = 0;
            }
        }
    }
    return cleared;
}
