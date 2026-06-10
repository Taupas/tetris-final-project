#include "../include/block_puzzle_api.h"
#include "../include/board.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    int count;
    int dr[BLOCK_MAX_SHAPE];
    int dc[BLOCK_MAX_SHAPE];
} ShapeDef;

typedef struct {
    Board *board;
    int slot_len[BLOCK_SLOTS];
    int slot_dr[BLOCK_SLOTS][BLOCK_MAX_SHAPE];
    int slot_dc[BLOCK_SLOTS][BLOCK_MAX_SHAPE];
    int slot_active[BLOCK_SLOTS];
    unsigned int rng;
    int score;
    int theme;
    int theme_changed;
    int game_over;
} BlockGame;

static BlockGame *block_slots[MAX_GAMES];

static const ShapeDef SHAPE_POOL[] = {
    {1, {0}, {0}},
    {2, {0, 0}, {0, 1}},
    {2, {0, 1}, {0, 0}},
    {3, {0, 0, 0}, {0, 1, 2}},
    {3, {0, 1, 2}, {0, 0, 0}},
    {4, {0, 0, 0, 0}, {0, 1, 2, 3}},
    {4, {0, 1, 2, 3}, {0, 0, 0, 0}},
    {4, {0, 0, 1, 1}, {0, 1, 0, 1}},
    {3, {0, 0, 1}, {0, 1, 1}},
    {3, {0, 1, 1}, {0, 0, 0}},
    {4, {0, 0, 1, 1}, {0, 1, 1, 2}},
    {4, {0, 1, 1, 2}, {0, 0, 1, 1}},
    {4, {0, 0, 0, 1}, {1, 1, 2, 1}},
    {5, {0, 0, 1, 1, 2}, {0, 1, 0, 1, 0}},
    {6, {0, 0, 1, 1, 2, 2}, {0, 1, 0, 1, 0, 1}},
};

static const int SHAPE_POOL_SIZE = (int)(sizeof(SHAPE_POOL) / sizeof(SHAPE_POOL[0]));

static unsigned int block_rand(unsigned int *state) {
    unsigned int x = *state;
    if (x == 0) {
        x = 123456789u;
    }
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static BlockGame *block_get(int game_id) {
    if (game_id < 0 || game_id >= MAX_GAMES) {
        return NULL;
    }
    return block_slots[game_id];
}

static void free_block_game(BlockGame *g) {
    if (!g) {
        return;
    }
    board_destroy(g->board);
    free(g);
}

static int board_is_empty(const BlockGame *g) {
    int r, c;
    if (!g || !g->board) {
        return 1;
    }
    for (r = 0; r < BLOCK_GRID; r++) {
        for (c = 0; c < BLOCK_GRID; c++) {
            if (board_get(g->board, r, c) != 0) {
                return 0;
            }
        }
    }
    return 1;
}

static void set_theme(BlockGame *g, int random_pick) {
    if (random_pick) {
        g->theme = (int)(block_rand(&g->rng) % (unsigned int)BLOCK_THEME_COUNT);
    } else {
        g->theme = (g->theme + 1) % BLOCK_THEME_COUNT;
    }
    g->theme_changed = 1;
}

static void normalize_shape_coords(int n, int *dr, int *dc) {
    int i, min_r, min_c;
    if (n <= 0) {
        return;
    }
    min_r = dr[0];
    min_c = dc[0];
    for (i = 1; i < n; i++) {
        if (dr[i] < min_r) {
            min_r = dr[i];
        }
        if (dc[i] < min_c) {
            min_c = dc[i];
        }
    }
    for (i = 0; i < n; i++) {
        dr[i] -= min_r;
        dc[i] -= min_c;
    }
}

static void assign_shape(BlockGame *g, int slot) {
    int idx = (int)(block_rand(&g->rng) % (unsigned int)SHAPE_POOL_SIZE);
    const ShapeDef *s = &SHAPE_POOL[idx];
    int i;
    g->slot_len[slot] = s->count;
    for (i = 0; i < s->count; i++) {
        g->slot_dr[slot][i] = s->dr[i];
        g->slot_dc[slot][i] = s->dc[i];
    }
    normalize_shape_coords(g->slot_len[slot], g->slot_dr[slot], g->slot_dc[slot]);
    g->slot_active[slot] = 1;
}

static void refill_slots(BlockGame *g) {
    int i;
    for (i = 0; i < BLOCK_SLOTS; i++) {
        assign_shape(g, i);
    }
}

static int can_place(const BlockGame *g, int slot, int row, int col) {
    int i;
    if (!g || !g->board || slot < 0 || slot >= BLOCK_SLOTS || !g->slot_active[slot]) {
        return 0;
    }
    for (i = 0; i < g->slot_len[slot]; i++) {
        int r = row + g->slot_dr[slot][i];
        int c = col + g->slot_dc[slot][i];
        if (r < 0 || r >= BLOCK_GRID || c < 0 || c >= BLOCK_GRID) {
            return 0;
        }
        if (board_get(g->board, r, c) != 0) {
            return 0;
        }
    }
    return 1;
}

/* 依滑鼠目標格對齊：先精確錨點，再對齊到目標格上的方塊格，最後小範圍搜尋 */
static int resolve_placement(const BlockGame *g, int slot, int target_r, int target_c,
                             int *out_r, int *out_c) {
    int try_r, try_c, i, dr, dc, radius;

    if (can_place(g, slot, target_r, target_c)) {
        *out_r = target_r;
        *out_c = target_c;
        return 1;
    }

    for (try_r = 0; try_r < BLOCK_GRID; try_r++) {
        for (try_c = 0; try_c < BLOCK_GRID; try_c++) {
            if (!can_place(g, slot, try_r, try_c)) {
                continue;
            }
            for (i = 0; i < g->slot_len[slot]; i++) {
                if (try_r + g->slot_dr[slot][i] == target_r &&
                    try_c + g->slot_dc[slot][i] == target_c) {
                    *out_r = try_r;
                    *out_c = try_c;
                    return 1;
                }
            }
        }
    }

    for (radius = 1; radius <= 3; radius++) {
        for (dr = -radius; dr <= radius; dr++) {
            for (dc = -radius; dc <= radius; dc++) {
                int manhattan = dr < 0 ? -dr : dr;
                int dc_abs = dc < 0 ? -dc : dc;
                manhattan += dc_abs;
                if (manhattan > radius) {
                    continue;
                }
                try_r = target_r + dr;
                try_c = target_c + dc;
                if (try_r >= 0 && try_r < BLOCK_GRID && try_c >= 0 && try_c < BLOCK_GRID &&
                    can_place(g, slot, try_r, try_c)) {
                    *out_r = try_r;
                    *out_c = try_c;
                    return 1;
                }
            }
        }
    }

    return 0;
}

static int clear_lines(BlockGame *g) {
    int cleared = 0;
    int r, c;

    if (!g || !g->board) {
        return 0;
    }

    for (r = 0; r < BLOCK_GRID; r++) {
        int full = 1;
        for (c = 0; c < BLOCK_GRID; c++) {
            if (board_get(g->board, r, c) == 0) {
                full = 0;
                break;
            }
        }
        if (full) {
            cleared++;
            for (c = 0; c < BLOCK_GRID; c++) {
                board_set(g->board, r, c, 0);
            }
        }
    }

    for (c = 0; c < BLOCK_GRID; c++) {
        int full = 1;
        for (r = 0; r < BLOCK_GRID; r++) {
            if (board_get(g->board, r, c) == 0) {
                full = 0;
                break;
            }
        }
        if (full) {
            cleared++;
            for (r = 0; r < BLOCK_GRID; r++) {
                board_set(g->board, r, c, 0);
            }
        }
    }

    if (cleared > 0 && board_is_empty(g)) {
        set_theme(g, 0);
        g->score += 200;
    }
    return cleared;
}

static int any_placement(const BlockGame *g, int slot) {
    int row, col;
    if (!g->slot_active[slot]) {
        return 1;
    }
    for (row = 0; row < BLOCK_GRID; row++) {
        for (col = 0; col < BLOCK_GRID; col++) {
            if (can_place(g, slot, row, col)) {
                return 1;
            }
        }
    }
    return 0;
}

static int count_active_slots(const BlockGame *g) {
    int i, n = 0;
    for (i = 0; i < BLOCK_SLOTS; i++) {
        if (g->slot_active[i]) {
            n++;
        }
    }
    return n;
}

static void check_game_over(BlockGame *g) {
    int i;
    int any_can_place = 0;

    if (count_active_slots(g) == 0) {
        refill_slots(g);
    }

    /* 只要還有任何一塊「現有」方塊能放，遊戲就繼續（不是某一塊放不下就結束） */
    for (i = 0; i < BLOCK_SLOTS; i++) {
        if (!g->slot_active[i]) {
            continue;
        }
        if (any_placement(g, i)) {
            any_can_place = 1;
            break;
        }
    }

    g->game_over = any_can_place ? 0 : 1;
}

static void fill_state(const BlockGame *g, BlockEngineState *out) {
    int r, c, s, i;
    memset(out, 0, sizeof(BlockEngineState));
    for (r = 0; r < BLOCK_GRID; r++) {
        for (c = 0; c < BLOCK_GRID; c++) {
            out->board[r * BLOCK_GRID + c] = board_get(g->board, r, c);
        }
    }
    for (s = 0; s < BLOCK_SLOTS; s++) {
        out->slot_len[s] = g->slot_len[s];
        out->slot_active[s] = g->slot_active[s];
        for (i = 0; i < BLOCK_MAX_SHAPE; i++) {
            out->slot_dr[s * BLOCK_MAX_SHAPE + i] = g->slot_dr[s][i];
            out->slot_dc[s * BLOCK_MAX_SHAPE + i] = g->slot_dc[s][i];
        }
    }
    out->score = g->score;
    out->theme = g->theme;
    out->theme_changed = g->theme_changed;
    out->game_over = g->game_over;
}

API_EXPORT int block_create(unsigned int seed) {
    int id;
    BlockGame *g;

    for (id = 0; id < MAX_GAMES; id++) {
        if (block_slots[id] == NULL) {
            break;
        }
    }
    if (id >= MAX_GAMES) {
        return -1;
    }

    g = (BlockGame *)malloc(sizeof(BlockGame));
    if (!g) {
        return -1;
    }
    memset(g, 0, sizeof(BlockGame));

    g->board = board_create(BLOCK_GRID, BLOCK_GRID);
    if (!g->board) {
        free(g);
        return -1;
    }

    g->rng = seed ? seed : (unsigned int)time(NULL);
    set_theme(g, 1);
    refill_slots(g);
    check_game_over(g);

    block_slots[id] = g;
    return id;
}

API_EXPORT void block_destroy(int game_id) {
    BlockGame *g = block_get(game_id);
    if (g) {
        free_block_game(g);
        block_slots[game_id] = NULL;
    }
}

API_EXPORT int block_place(int game_id, int slot, int row, int col) {
    BlockGame *g = block_get(game_id);
    int i, cleared, need_refill;
    if (!g || g->game_over) {
        return 0;
    }
    int place_r, place_c;

    g->theme_changed = 0;
    if (!resolve_placement(g, slot, row, col, &place_r, &place_c)) {
        return 0;
    }
    row = place_r;
    col = place_c;
    for (i = 0; i < g->slot_len[slot]; i++) {
        int r = row + g->slot_dr[slot][i];
        int c = col + g->slot_dc[slot][i];
        board_set(g->board, r, c, 1);
    }
    g->slot_active[slot] = 0;
    cleared = clear_lines(g);
    g->score += cleared * cleared * 10 + cleared * 50;
    need_refill = 1;
    for (i = 0; i < BLOCK_SLOTS; i++) {
        if (g->slot_active[i]) {
            need_refill = 0;
            break;
        }
    }
    if (need_refill) {
        refill_slots(g);
    }
    check_game_over(g);
    return 1;
}

API_EXPORT int block_get_state(int game_id, BlockEngineState *out) {
    BlockGame *g = block_get(game_id);
    if (!g || !out) {
        return 0;
    }
    fill_state(g, out);
    g->theme_changed = 0;
    return 1;
}
