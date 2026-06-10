#include "../include/physics.h"

int physics_can_place(const Board *b, const Piece *p) {
    int rows[4], cols[4];
    int i;
    piece_get_cells(p->type, p->rotation, p->x, p->y, rows, cols);
    for (i = 0; i < 4; i++) {
        if (rows[i] < 0) {
            continue;
        }
        if (cols[i] < 0 || cols[i] >= BOARD_WIDTH || rows[i] >= BOARD_HEIGHT) {
            return 0;
        }
        if (rows[i] >= 0 && board_get(b, rows[i], cols[i]) > 0) {
            return 0;
        }
    }
    return 1;
}

int physics_move(Board *b, Piece *p, int dx, int dy) {
    Piece trial = *p;
    trial.x += dx;
    trial.y += dy;
    if (physics_can_place(b, &trial)) {
        *p = trial;
        return 1;
    }
    return 0;
}

static int try_rotate(Board *b, Piece *p, int new_rot, int kick_dx) {
    Piece trial = *p;
    trial.rotation = new_rot;
    trial.x += kick_dx;
    if (physics_can_place(b, &trial)) {
        *p = trial;
        return 1;
    }
    return 0;
}

int physics_rotate(Board *b, Piece *p, int direction) {
    int new_rot = (p->rotation + direction + 4) % 4;
    int kicks[] = {0, -1, 1, -2, 2};
    int i;
    for (i = 0; i < 5; i++) {
        if (try_rotate(b, p, new_rot, kicks[i])) {
            return 1;
        }
    }
    return 0;
}

void physics_lock(Board *b, const Piece *p) {
    int rows[4], cols[4];
    int i;
    int color = piece_type_color(p->type);
    piece_get_cells(p->type, p->rotation, p->x, p->y, rows, cols);
    for (i = 0; i < 4; i++) {
        if (rows[i] >= 0 && rows[i] < BOARD_HEIGHT && cols[i] >= 0 && cols[i] < BOARD_WIDTH) {
            board_set(b, rows[i], cols[i], color);
        }
    }
}

int physics_hard_drop(Board *b, Piece *p) {
    int dropped = 0;
    while (physics_move(b, p, 0, 1)) {
        dropped++;
    }
    return dropped;
}
