#include "../include/engine_api.h"
#include "../include/board.h"
#include "../include/tetromino.h"
#include "../include/physics.h"
#include "../include/score.h"
#include "../include/queue.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    Board *board;
    Piece current;
    int next_type;
    IntQueue *piece_queue;
    unsigned int rng;
    int score;
    int lines;
    int level;
    int game_over;
} Game;

static Game *game_slots[MAX_GAMES];

static unsigned int xorshift32(unsigned int *state) {
    unsigned int x = *state;
    if (x == 0) {
        x = 2463534242u;
    }
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void refill_piece_queue(Game *g) {
    int bag[7];
    int i;
    for (i = 0; i < 7; i++) {
        bag[i] = i;
    }
    for (i = 6; i > 0; i--) {
        int j = (int)(xorshift32(&g->rng) % (unsigned int)(i + 1));
        int tmp = bag[i];
        bag[i] = bag[j];
        bag[j] = tmp;
    }
    queue_clear(g->piece_queue);
    for (i = 0; i < 7; i++) {
        queue_push(g->piece_queue, bag[i]);
    }
}

static int next_piece_type(Game *g) {
    if (queue_is_empty(g->piece_queue)) {
        refill_piece_queue(g);
    }
    return queue_pop(g->piece_queue);
}

static int spawn_piece(Game *g) {
    int type = g->next_type;
    g->next_type = next_piece_type(g);
    g->current.type = type;
    g->current.rotation = 0;
    g->current.x = 5;
    g->current.y = 0;
    if (!physics_can_place(g->board, &g->current)) {
        g->game_over = 1;
        return 0;
    }
    return 1;
}

static void lock_and_continue(Game *g) {
    int cleared;
    physics_lock(g->board, &g->current);
    cleared = board_clear_lines(g->board);
    if (cleared > 0) {
        score_add_lines(&g->score, &g->lines, &g->level, cleared);
    }
    spawn_piece(g);
}

static Game *get_game(int game_id) {
    if (game_id < 0 || game_id >= MAX_GAMES) {
        return NULL;
    }
    return game_slots[game_id];
}

static void fill_state(const Game *g, EngineState *out) {
    int r, c, i;
    memset(out, 0, sizeof(EngineState));
    for (r = 0; r < BOARD_HEIGHT; r++) {
        for (c = 0; c < BOARD_WIDTH; c++) {
            out->board[r * BOARD_WIDTH + c] = board_get(g->board, r, c);
        }
    }
    if (!g->game_over) {
        int rows[4], cols[4];
        piece_get_cells(g->current.type, g->current.rotation, g->current.x, g->current.y, rows, cols);
        out->active_count = 0;
        for (i = 0; i < 4; i++) {
            if (rows[i] >= 0 && rows[i] < BOARD_HEIGHT && cols[i] >= 0 && cols[i] < BOARD_WIDTH) {
                out->active_rows[out->active_count] = rows[i];
                out->active_cols[out->active_count] = cols[i];
                out->active_count++;
            }
        }
        out->active_type = g->current.type;
    }
    out->next_type = g->next_type;
    out->score = g->score;
    out->lines = g->lines;
    out->level = g->level;
    out->game_over = g->game_over;
}

static void free_game(Game *g) {
    if (!g) {
        return;
    }
    board_destroy(g->board);
    queue_destroy(g->piece_queue);
    free(g);
}

API_EXPORT int engine_create(unsigned int seed) {
    int id;
    Game *g;

    for (id = 0; id < MAX_GAMES; id++) {
        if (game_slots[id] == NULL) {
            break;
        }
    }
    if (id >= MAX_GAMES) {
        return -1;
    }

    g = (Game *)malloc(sizeof(Game));
    if (!g) {
        return -1;
    }
    memset(g, 0, sizeof(Game));

    g->board = board_create(BOARD_HEIGHT, BOARD_WIDTH);
    g->piece_queue = queue_create(7);
    if (!g->board || !g->piece_queue) {
        free_game(g);
        return -1;
    }

    g->rng = seed ? seed : (unsigned int)time(NULL);
    g->level = 1;
    refill_piece_queue(g);
    g->next_type = next_piece_type(g);
    spawn_piece(g);

    game_slots[id] = g;
    return id;
}

API_EXPORT void engine_destroy(int game_id) {
    Game *g = get_game(game_id);
    if (g) {
        free_game(g);
        game_slots[game_id] = NULL;
    }
}

API_EXPORT int engine_tick(int game_id) {
    Game *g = get_game(game_id);
    if (!g || g->game_over) {
        return 0;
    }
    if (!physics_move(g->board, &g->current, 0, 1)) {
        lock_and_continue(g);
    }
    return 1;
}

API_EXPORT int engine_move_left(int game_id) {
    Game *g = get_game(game_id);
    if (!g || g->game_over) {
        return 0;
    }
    return physics_move(g->board, &g->current, -1, 0);
}

API_EXPORT int engine_move_right(int game_id) {
    Game *g = get_game(game_id);
    if (!g || g->game_over) {
        return 0;
    }
    return physics_move(g->board, &g->current, 1, 0);
}

API_EXPORT int engine_soft_drop(int game_id) {
    Game *g = get_game(game_id);
    if (!g || g->game_over) {
        return 0;
    }
    if (physics_move(g->board, &g->current, 0, 1)) {
        g->score += score_drop_points(g->level);
        return 1;
    }
    lock_and_continue(g);
    return 0;
}

API_EXPORT int engine_rotate_cw(int game_id) {
    Game *g = get_game(game_id);
    if (!g || g->game_over) {
        return 0;
    }
    return physics_rotate(g->board, &g->current, 1);
}

API_EXPORT int engine_rotate_ccw(int game_id) {
    Game *g = get_game(game_id);
    if (!g || g->game_over) {
        return 0;
    }
    return physics_rotate(g->board, &g->current, -1);
}

API_EXPORT int engine_hard_drop(int game_id) {
    Game *g = get_game(game_id);
    int dropped;
    if (!g || g->game_over) {
        return 0;
    }
    dropped = physics_hard_drop(g->board, &g->current);
    g->score += dropped * 2 * score_drop_points(g->level);
    lock_and_continue(g);
    return dropped;
}

API_EXPORT int engine_get_state(int game_id, EngineState *out) {
    Game *g = get_game(game_id);
    if (!g || !out) {
        return 0;
    }
    fill_state(g, out);
    return 1;
}
