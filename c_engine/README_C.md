# C 語言核心引擎 — 期末規範對照

## 必備項目

| 項目 | 實作位置 |
|------|----------|
| **指標** | `Game *`、`Board *`、`IntQueue *`、`board_get()` 等 |
| **malloc / free** | `engine_create` → `malloc(Game)`；`board_create` → `malloc(Board)` + `malloc(cells)`；`queue_create` → `malloc`；對應 `free` / `board_destroy` / `queue_destroy` |
| **struct** | `Board`、`Piece`、`Game`、`BlockGame`、`IntQueue`、`EngineState` |
| **函式拆分** | `board.c`、`physics.c`、`tetromino.c`、`score.c`、`queue.c`、`engine_api.c`、`block_puzzle_api.c` |

## 資料結構（擇一以上）

- **動態陣列**：`Board.cells` 以 `malloc(rows * cols * sizeof(int))` 配置
- **二維陣列概念**：以 `row * cols + col` 索引 10×20 / 8×8 棋盤

## 加分項目

- **queue**：`queue.c` 環形佇列，經典模式 7-bag 出塊使用 `queue_push` / `queue_pop`

## 編譯

```bat
build.bat
```

產生 `lib\tetris_engine.dll`
