#  Tetris Game System(Flask × C)

> **1142 學期 計算機程式語言 期末專題**  
> 以 Python Flask 作為 Web 前端、C 語言作為核心引擎，透過 **ctypes** 進行語言間整合。

**遊戲網址（本地）：**https://tetris-final-project.onrender.com/tetris
---

## 專題名稱與功能介紹

可在瀏覽器遊玩的俄羅斯方塊，提供兩種模式：

| 模式 | 說明 |
|------|------|
| **經典模式** | 10×20 標準下落式方塊，7-bag 隨機出塊，鍵盤操作，消行計分與等級 |
| **拼圖模式** | 8×8 棋盤，從托盤拖放方塊，消整行/整列得分，全盤清除可更換配色 |

**共同功能：** Canvas 即時繪圖、歷史分數排行榜、REST API。

**核心原則：** 棋盤、碰撞、出塊、消行、拼圖放置等遊戲邏輯 **100% 由 C 實作**；Flask 僅負責 HTTP 與 JSON，不參與遊戲判定。

---

## 使用技術

| 層級 | 技術 | 展示能力 |
|------|------|----------|
| 前端 UI | HTML + CSS + Vanilla JS (Canvas) | DOM、fetch、拖放 |
| Web 後端 | Python 3.10+ / Flask | REST、JSON |
| 語言整合 | **ctypes** | 同進程呼叫 C DLL，`EngineState` struct 傳遞 |
| 遊戲核心 | **C (C11)** | **pointer、malloc/free、struct、函式拆分** |
| 資料結構 | **動態陣列** + **環形 Queue** | `Board.cells`；7-bag 用 `IntQueue` |

---

## 一、專案整體架構設計

### 1.1 三層架構

```
┌──────────────────────────────────────────────────────────────┐
│  表現層   index.html · game.js · puzzle.js · style.css        │
│           使用者輸入 → fetch(/tetris/api/...)               │
└────────────────────────────┬─────────────────────────────────┘
                             │ HTTP + JSON
┌────────────────────────────▼─────────────────────────────────┐
│  應用層   app.py · engine_bridge.py                           │
│           路由、排行榜檔案、ctypes 型別綁定                    │
└────────────────────────────┬─────────────────────────────────┘
                             │ C ABI（in-process）
┌────────────────────────────▼─────────────────────────────────┐
│  核心層   lib/tetris_engine.dll                               │
│           engine_api.c (10×20)  block_puzzle_api.c (8×8)      │
│           board · physics · tetromino · queue · score         │
└──────────────────────────────────────────────────────────────┘
```

### 1.2 為何採用 ctypes（而非 subprocess）

| 方案 | 優點 | 本專題選擇 |
|------|------|------------|
| subprocess | 實作簡單 | 每次操作需啟動行程、序列化整盤 |
| **ctypes** | `Game*` 常駐記憶體、低延遲 | ✅ 與課程 demo 相同 |

### 1.3 模組拆分原則

- **單一職責：** `board` 只管格子、`physics` 只管碰撞、`score` 只管分數。
- **雙 API 入口：** `engine_api.c`（經典）與 `block_puzzle_api.c`（拼圖）共用 `board.c`。
- **薄 Flask 層：** `app.py` 不含遊戲規則，僅轉發至 DLL。

---

## 二、Pointer 與 Malloc 用法解析

### 2.1 棋盤配置（兩層 malloc）

```c
Board *board_create(int rows, int cols) {
    Board *b = (Board *)malloc(sizeof(Board));
    if (!b) return NULL;

    b->cells = (int *)malloc((size_t)rows * (size_t)cols * sizeof(int));
    if (!b->cells) {
        free(b);          /* 失敗時釋放已配置的 Board */
        return NULL;
    }
    b->rows = rows;
    b->cols = cols;
    return b;
}

void board_destroy(Board *b) {
    if (!b) return;
    free(b->cells);       /* 先釋放子資源 */
    free(b);              /* 再釋放結構本體 */
}
```

**重點：**

- `Board *` 為指向堆積上結構的指標。
- `cells` 為第二塊堆積記憶體，模擬二維棋盤。
- **free 順序與 malloc 相反**，避免 leak 與 dangling pointer。

### 2.2 格子存取（指標 + 索引）

```c
static int idx(const Board *b, int row, int col) {
    return row * b->cols + col;
}

int board_get(const Board *b, int row, int col) {
    return b->cells[idx(b, row, col)];
}
```

`const Board *` 表示唯讀存取，碰撞檢查時不會意外改盤。

### 2.3 環形佇列（7-bag 出塊）

```c
IntQueue *queue_create(int capacity) {
    IntQueue *q = (IntQueue *)malloc(sizeof(IntQueue));
    q->data = (int *)malloc((size_t)capacity * sizeof(int));
    /* ... */
    return q;
}
```

`engine_create` 內配置 `Game *` → 內含 `Board *`、`IntQueue *`；`engine_destroy` 依序 `board_destroy` → `queue_destroy` → `free(Game)`。

### 2.4 malloc / free 對照表

| 配置 | 釋放 | 檔案 |
|------|------|------|
| `malloc(sizeof(Board))` + `malloc(cells)` | `board_destroy` | `board.c` |
| `malloc(sizeof(IntQueue))` + `malloc(data)` | `queue_destroy` | `queue.c` |
| `malloc(sizeof(Game))` | `free_game` | `engine_api.c` |
| `malloc(sizeof(BlockGame))` | `free_block_game` | `block_puzzle_api.c` |

---

## 三、資料結構的應用場景與設計理由

### 3.1 `Board` — 動態一維陣列模擬二維棋盤（必備）

```c
typedef struct {
    int *cells;
    int rows;
    int cols;
} Board;
```

| 場景 | 設計理由 |
|------|----------|
| 經典 10×20 | `board_create(20, 10)` |
| 拼圖 8×8 | `board_create(8, 8)` 重用同一套 API |
| ctypes 傳遞 | `EngineState.board[]` 扁平陣列與 `row*cols+col` 一致 |

### 3.2 `IntQueue` — 環形佇列（加分：queue）

| 場景 | 設計理由 |
|------|----------|
| 7-bag 出塊 | 將 0–6 七種方塊洗牌後 push，pop 保證公平隨機 |
| 環形索引 | `head` + 模運算，push/pop O(1)，不需搬移陣列 |

### 3.3 `Piece` / `Game` — 遊戲狀態封裝

- `Piece`：目前方塊類型、旋轉、座標。
- `Game`：聚合 `Board *`、`Piece`、`IntQueue *`、分數與 RNG。
- `game_slots[MAX_GAMES]`：以 `game_id` 支援多局（Flask 每玩家一個 id）。

### 3.4 `EngineState` / `BlockEngineState` — 跨語言邊界

C 端輸出**不含指標**的扁平 struct，供 Python `ctypes.Structure` 直接讀取，避免跨語言傳遞堆積指標。

---

## 四、系統架構與執行方式

### 4.1 目錄結構

```
tetris/
├── c_engine/include/     # .h 標頭與 struct 定義
├── c_engine/src/         # .c 實作
├── flask_app/
│   ├── app.py
│   ├── services/engine_bridge.py
│   ├── templates/index.html
│   └── static/{css,js}/
├── lib/tetris_engine.dll # build.bat 產生
├── build.bat · run.bat
├── requirements.txt
└── submission/           # 繳交文件
```

完整說明見 [`submission/02_CODE_STRUCTURE.md`](submission/02_CODE_STRUCTURE.md)。

### 4.2 如何執行 (How to run)

```bat
cd tetris
run.bat
```

瀏覽器開啟：**http://127.0.0.1:5000/tetris**

手動步驟：

```bat
build.bat
pip install -r requirements.txt
cd flask_app
python app.py
```

**環境需求：** Python 3.10+、MinGW-w64（`gcc` 在 PATH）

**常見問題：**

| 狀況 | 解法 |
|------|------|
| 找不到 gcc | 安裝 Git for Windows / MSYS2，將 `gcc` 加入 PATH |
| DLL Permission denied | 關閉執行中的 Flask 再 `build.bat` |
| 無法連線 | 確認終端機中 `python app.py` 仍在執行 |

---

## 五、對外 API

### 5.1 C ↔ Python（DLL）

**經典模式**

| C 函式 | 說明 |
|--------|------|
| `engine_create` / `engine_destroy` | 建立 / 釋放遊戲 |
| `engine_tick` | 重力下落 |
| `engine_move_*` / `engine_rotate_*` / `engine_hard_drop` | 玩家操作 |
| `engine_get_state` | 輸出 `EngineState` |

**拼圖模式**

| C 函式 | 說明 |
|--------|------|
| `block_create` / `block_destroy` | 建立 / 釋放 |
| `block_place` | 放置方塊（含 `resolve_placement` 貼邊對齊） |
| `block_get_state` | 輸出 `BlockEngineState` |

### 5.2 HTTP Routes

| Method | Path | 說明 |
|--------|------|------|
| GET | `/tetris` | 遊戲頁面 |
| GET | `/` | 導向 `/tetris` |
| POST | `/tetris/api/new` | 新經典局 |
| POST | `/tetris/api/input` | `{game_id, action}` |
| POST | `/tetris/api/tick` | 重力 tick |
| GET/POST | `/tetris/api/leaderboard` | 經典排行榜 |
| POST | `/tetris/api/block/new` | 新拼圖局 |
| POST | `/tetris/api/block/place` | `{game_id, slot, row, col}` |
| GET/POST | `/tetris/api/puzzle/leaderboard` | 拼圖排行榜 |

---

## 六、操作方式

### 經典模式

| 按鍵 | 功能 |
|------|------|
| ← → | 左右移動 |
| ↑ | 旋轉 |
| ↓ | 軟降 |
| 空白鍵 | 硬降 |
| P | 暫停 |

### 拼圖模式

- 拖放托盤方塊至棋盤
- 綠色預覽 = 可放置；紅色 = 不可放置
- 三塊皆無法放置時才結束

---

## 七、你與 Cursor 協作解決的具體問題

| 問題 | 解法 |
|------|------|
| 從零建立 Flask + C 分層 | 架構設計 → `c_engine` + `engine_bridge.py` + `build.bat` |
| 拼圖方塊不顯示 | ctypes 改扁平陣列讀 `slot_dr` / `slot_dc` |
| 過早 game over | 改為「托盤上**全部**方塊都放不下」才結束 |
| L 形無法貼邊 | `resolve_placement` + 方塊中心對齊滑鼠 |
| 遊戲路徑 | 統一為 `/tetris` 前綴 |
| 期末繳交文件 | README、程式碼結構、Cursor 紀錄 |

### Cursor Prompt 使用紀錄（摘要）

**1. 規劃與架構**

> 「請依 Flask + C 架構完成期末專題，含 pointer、malloc、struct、queue。」

**2. 逐步實作**

> 「設計 `Board` 與 `board_create`，用 malloc 配置動態棋盤。」  
> 「以環形佇列實作 7-bag 出塊。」

**3. 精準除錯**

> 「拼圖方塊不顯示、game over 過早。」  
> 「∟ 形方塊無法放到邊邊。」

**4. 深度理解**

> 「解釋 `Board *` 與 `cells` 索引及 free 順序。」

完整對話整理見 [`submission/04_CURSOR_PROMPT_RECORD.md`](submission/04_CURSOR_PROMPT_RECORD.md)。

---


## 授權

教學與課程繳交用途 · MIT License

