# 繳交項目四：Cursor Prompt 對話紀錄

> **PDF 繳交版：** [`Cursor Prompt.pdf`](Cursor%20Prompt.pdf)（與課堂範本相同格式）  
> **純文字版：** [`Cursor_Prompt_內容.txt`](Cursor_Prompt_內容.txt)

完整 Markdown 說明見專案開發過程；繳交時可直接上傳 **Cursor Prompt.pdf**。

---

## 快速對照

| 階段 | 內容 |
|------|------|
| 第一步 | C 核心引擎 `c_engine/`（Board、Queue、engine_api、block_puzzle_api） |
| 第二步 | Flask + ctypes `engine_bridge.py`、`/tetris/api/*` |
| 第三步 | 經典 + 拼圖雙模式前端 |
| 第四步 | `build.bat`、`run.bat` → http://127.0.0.1:5000/tetris |
| 第五步 | `requirements.txt`（flask） |
| 除錯紀錄 | 方塊不顯示、game over、L 形貼邊、環境安裝 |

詳細 Prompt 原文請開啟 **Cursor Prompt.pdf**。
