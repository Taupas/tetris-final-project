#include "../include/score.h"

void score_add_lines(int *score, int *lines, int *level, int cleared) {
    static const int table[5] = {0, 100, 300, 500, 800};
    int idx = cleared;
    if (idx < 0) {
        idx = 0;
    }
    if (idx > 4) {
        idx = 4;
    }
    if (score && lines && level) {
        *score += table[idx] * (*level);
        *lines += cleared;
        *level = 1 + (*lines / 10);
        if (*level > 20) {
            *level = 20;
        }
    }
}

int score_drop_points(int level) {
    return level;
}
