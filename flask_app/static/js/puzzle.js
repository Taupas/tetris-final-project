/* 拼圖模式：Flask API + C 引擎（block_puzzle_api） */
(function () {
  "use strict";

  const SIZE = 8;
  const COLOR_SETS = [
    { wrap: "linear-gradient(180deg, #c8b8e8 0%, #9888c0 100%)", cell: "#b0a0c8", block: "#4a2878", hi: "#7a58a8", edge: "#fff" },
    { wrap: "linear-gradient(180deg, #b8e0c8 0%, #78b090 100%)", cell: "#a0d0b0", block: "#1a6840", hi: "#48a070", edge: "#fff" },
    { wrap: "linear-gradient(180deg, #b8d0f0 0%, #78a0c8 100%)", cell: "#a0c0e0", block: "#184878", hi: "#4878a8", edge: "#fff" },
    { wrap: "linear-gradient(180deg, #f0d8b0 0%, #d0a070 100%)", cell: "#e8c898", block: "#a05810", hi: "#d08830", edge: "#fff" },
    { wrap: "linear-gradient(180deg, #f0b8d0 0%, #d07098 100%)", cell: "#e8a0b8", block: "#a02850", hi: "#d05878", edge: "#fff" },
    { wrap: "linear-gradient(180deg, #e0b8f0 0%, #b078c8 100%)", cell: "#d0a0e0", block: "#7828a0", hi: "#a858c8", edge: "#fff" },
    { wrap: "linear-gradient(180deg, #b8f0e8 0%, #68c0b0 100%)", cell: "#a8e8d8", block: "#188870", hi: "#48b8a0", edge: "#fff" },
    { wrap: "linear-gradient(180deg, #f0e8b0 0%, #d0c068 100%)", cell: "#e8e0a0", block: "#a08818", hi: "#d0b848", edge: "#fff" },
  ];

  let boardEl, boardCtx, scoreEl, statusEl, wrapEl;
  let leaderboardEl, scoreForm, playerNameInput;
  let pieceCanvases = [];

  let gameId = null;
  let state = null;
  let scoreSubmitted = false;
  let theme = COLOR_SETS[0];
  let cellPx = 40;
  let drag = null;
  let ghost = null;
  let lastSnap = null;

  function pickTheme(index) {
    theme = COLOR_SETS[((index % COLOR_SETS.length) + COLOR_SETS.length) % COLOR_SETS.length];
    if (wrapEl) wrapEl.style.background = theme.wrap;
  }

  function apiUrl(path) {
    const base = typeof window !== "undefined" && window.APP_BASE ? window.APP_BASE : "";
    return path.startsWith("http") ? path : base + path;
  }

  async function api(path, options = {}) {
    const res = await fetch(apiUrl(path), {
      headers: { "Content-Type": "application/json" },
      ...options,
    });
    if (!res.ok) throw new Error((await res.text()) || res.statusText);
    return res.json();
  }

  function shapeSize(cells) {
    let mr = 0, mc = 0;
    cells.forEach(([r, c]) => {
      if (r > mr) mr = r;
      if (c > mc) mc = c;
    });
    return { rows: mr + 1, cols: mc + 1 };
  }

  function shapeCenterOffset(cells) {
    let minR = Infinity, minC = Infinity, maxR = 0, maxC = 0;
    cells.forEach(([r, c]) => {
      if (r < minR) minR = r;
      if (c < minC) minC = c;
      if (r > maxR) maxR = r;
      if (c > maxC) maxC = c;
    });
    return { dr: (minR + maxR) / 2, dc: (minC + maxC) / 2 };
  }

  /** 滑鼠對準方塊視覺中心（與托盤預覽一致），而非形狀座標 (0,0) */
  function placementTargetFromPointer(cells, p) {
    const c = shapeCenterOffset(cells);
    return {
      row: Math.round(p.row - c.dr),
      col: Math.round(p.col - c.dc),
    };
  }

  function normalizeCells(cells) {
    if (!cells?.length) return cells || [];
    let minR = cells[0][0], minC = cells[0][1];
    cells.forEach(([r, c]) => {
      if (r < minR) minR = r;
      if (c < minC) minC = c;
    });
    return cells.map(([r, c]) => [r - minR, c - minC]);
  }

  function drawBlock(ctx, x, y, sz) {
    const p = 2;
    const s = sz - p * 2;
    ctx.fillStyle = theme.block;
    ctx.fillRect(x + p, y + p, s, s);
    ctx.fillStyle = theme.hi;
    ctx.fillRect(x + p, y + p, s, s * 0.25);
    ctx.strokeStyle = theme.edge;
    ctx.lineWidth = 2;
    ctx.strokeRect(x + p + 0.5, y + p + 0.5, s - 1, s - 1);
  }

  function drawCellsOnCanvas(ctx, cells, canvasW, canvasH) {
    ctx.clearRect(0, 0, canvasW, canvasH);
    const { rows, cols } = shapeSize(cells);
    const pad = 8;
    const cellSz = Math.floor(Math.min((canvasW - pad * 2) / cols, (canvasH - pad * 2) / rows));
    const offX = Math.floor((canvasW - cols * cellSz) / 2);
    const offY = Math.floor((canvasH - rows * cellSz) / 2);
    cells.forEach(([dr, dc]) => {
      drawBlock(ctx, offX + dc * cellSz, offY + dr * cellSz, cellSz);
    });
  }

  function canPlaceOnBoard(board, cells, row, col) {
    if (!board) return false;
    for (const [dr, dc] of cells) {
      const r = row + dr;
      const c = col + dc;
      if (r < 0 || r >= SIZE || c < 0 || c >= SIZE) return false;
      if (board[r][c]) return false;
    }
    return true;
  }

  /** 與 C 引擎 resolve_placement 相同：對齊滑鼠下的格子，方便 L 形貼邊放置 */
  function resolvePlacement(cells, targetR, targetC, board) {
    if (canPlaceOnBoard(board, cells, targetR, targetC)) {
      return { row: targetR, col: targetC, ok: true };
    }
    for (let tryR = 0; tryR < SIZE; tryR++) {
      for (let tryC = 0; tryC < SIZE; tryC++) {
        if (!canPlaceOnBoard(board, cells, tryR, tryC)) continue;
        for (const [dr, dc] of cells) {
          if (tryR + dr === targetR && tryC + dc === targetC) {
            return { row: tryR, col: tryC, ok: true };
          }
        }
      }
    }
    for (let radius = 1; radius <= 3; radius++) {
      for (let dr = -radius; dr <= radius; dr++) {
        for (let dc = -radius; dc <= radius; dc++) {
          if (Math.abs(dr) + Math.abs(dc) > radius) continue;
          const tryR = targetR + dr;
          const tryC = targetC + dc;
          if (tryR >= 0 && tryR < SIZE && tryC >= 0 && tryC < SIZE &&
              canPlaceOnBoard(board, cells, tryR, tryC)) {
            return { row: tryR, col: tryC, ok: true };
          }
        }
      }
    }
    const center = shapeCenterOffset(cells);
    let best = null;
    let bestDist = Infinity;
    for (let tryR = 0; tryR < SIZE; tryR++) {
      for (let tryC = 0; tryC < SIZE; tryC++) {
        if (!canPlaceOnBoard(board, cells, tryR, tryC)) continue;
        const pr = tryR + center.dr;
        const pc = tryC + center.dc;
        const dist = (pr - targetR) ** 2 + (pc - targetC) ** 2;
        if (dist < bestDist) {
          bestDist = dist;
          best = { row: tryR, col: tryC };
        }
      }
    }
    if (best && bestDist <= 9) {
      return { row: best.row, col: best.col, ok: true };
    }
    return { row: targetR, col: targetC, ok: false };
  }

  function updateGhostFromPointer(ev) {
    if (!drag) return;
    const p = boardPos(ev);
    const target = placementTargetFromPointer(drag.cells, p);
    const snap = resolvePlacement(drag.cells, target.row, target.col, state?.board);
    lastSnap = snap;
    ghost = { row: snap.row, col: snap.col, cells: drag.cells, ok: snap.ok };
    drawBoard();
  }

  function slotCanFitAnywhere(cells, board) {
    for (let r = 0; r < SIZE; r++) {
      for (let c = 0; c < SIZE; c++) {
        if (canPlaceOnBoard(board, cells, r, c)) return true;
      }
    }
    return false;
  }

  /** 僅在「每一塊剩餘方塊都」無法放置時才結束 */
  function normalizeGameOver(st) {
    const slots = st.slots || [];
    const active = slots.filter((s) => s.active && s.cells && s.cells.length > 0);
    if (active.length === 0) {
      st.game_over = false;
      return st;
    }
    const anyFits = active.some((s) => slotCanFitAnywhere(s.cells, st.board));
    st.game_over = !anyFits;
    return st;
  }

  function canPlaceLocal(cells, row, col) {
    return canPlaceOnBoard(state?.board, cells, row, col);
  }

  function drawBoard() {
    if (!boardCtx || !state) return;
    const w = boardEl.width;
    const h = boardEl.height;
    boardCtx.clearRect(0, 0, w, h);
    cellPx = w / SIZE;

    for (let r = 0; r < SIZE; r++) {
      for (let c = 0; c < SIZE; c++) {
        const x = c * cellPx;
        const y = r * cellPx;
        boardCtx.fillStyle = theme.cell;
        boardCtx.fillRect(x, y, cellPx, cellPx);
        if (state.board[r][c]) drawBlock(boardCtx, x, y, cellPx);
      }
    }

    boardCtx.strokeStyle = "rgba(255,255,255,0.9)";
    boardCtx.lineWidth = 2;
    for (let i = 0; i <= SIZE; i++) {
      const p = Math.round(i * cellPx) + 0.5;
      boardCtx.beginPath();
      boardCtx.moveTo(p, 0);
      boardCtx.lineTo(p, h);
      boardCtx.stroke();
      boardCtx.beginPath();
      boardCtx.moveTo(0, p);
      boardCtx.lineTo(w, p);
      boardCtx.stroke();
    }

    boardCtx.strokeStyle = "rgba(0,0,0,0.4)";
    boardCtx.lineWidth = 1;
    for (let i = 0; i <= SIZE; i++) {
      const p = Math.round(i * cellPx);
      boardCtx.beginPath();
      boardCtx.moveTo(p, 0);
      boardCtx.lineTo(p, h);
      boardCtx.stroke();
      boardCtx.beginPath();
      boardCtx.moveTo(0, p);
      boardCtx.lineTo(w, p);
      boardCtx.stroke();
    }

    if (ghost?.cells?.length) {
      const ok = ghost.ok !== false;
      ghost.cells.forEach(([dr, dc]) => {
        const r = ghost.row + dr;
        const c = ghost.col + dc;
        if (r < 0 || r >= SIZE || c < 0 || c >= SIZE) return;
        boardCtx.fillStyle = ok ? "rgba(255,255,255,0.55)" : "rgba(255,40,40,0.6)";
        boardCtx.fillRect(c * cellPx + 3, r * cellPx + 3, cellPx - 6, cellPx - 6);
      });
    }
  }

  function renderTray() {
    pieceCanvases.forEach((cv, i) => {
      const slot = state?.slots?.[i];
      const ctx = cv.getContext("2d");
      if (!slot?.active || !slot.cells?.length) {
        ctx.clearRect(0, 0, cv.width, cv.height);
        cv.style.visibility = "hidden";
        return;
      }
      cv.style.visibility = "visible";
      drawCellsOnCanvas(ctx, slot.cells, cv.width, cv.height);
    });
  }

  function boardPos(ev) {
    const rect = boardEl.getBoundingClientRect();
    const sx = boardEl.width / rect.width;
    const sy = boardEl.height / rect.height;
    const x = (ev.clientX - rect.left) * sx;
    const y = (ev.clientY - rect.top) * sy;
    return {
      row: Math.max(0, Math.min(SIZE - 1, Math.floor(y / cellPx))),
      col: Math.max(0, Math.min(SIZE - 1, Math.floor(x / cellPx))),
    };
  }

  function applyState(newState) {
    if (newState?.slots) {
      newState.slots = newState.slots.map((s) => {
        if (!s?.cells?.length) return s;
        return { ...s, cells: normalizeCells(s.cells) };
      });
    }
    state = normalizeGameOver(newState);
    pickTheme(state.theme ?? 0);
    if (scoreEl) scoreEl.textContent = state.score;

    if (state.theme_changed && statusEl) {
      statusEl.textContent = "全盤清除！配色已更換";
      setTimeout(() => {
        if (state && !state.game_over && statusEl) statusEl.textContent = "";
      }, 2000);
    } else if (state.game_over) {
      if (statusEl) statusEl.textContent = "遊戲結束";
      showScoreForm();
    } else if (statusEl) {
      statusEl.textContent = "";
    }

    drawBoard();
    renderTray();
  }

  async function loadLeaderboard() {
    if (!leaderboardEl) return;
    try {
      const data = await api("/api/puzzle/leaderboard");
      leaderboardEl.innerHTML = "";
      const entries = data.entries || [];
      if (!entries.length) {
        leaderboardEl.innerHTML = "<li>尚無紀錄</li>";
        return;
      }
      entries.forEach((e, i) => {
        const li = document.createElement("li");
        li.textContent = `${i + 1}. ${e.name} — ${e.score}`;
        leaderboardEl.appendChild(li);
      });
    } catch (_) {
      leaderboardEl.innerHTML = "<li>無法載入排行榜</li>";
    }
  }

  function showScoreForm() {
    if (!scoreForm || scoreSubmitted || !state || state.score <= 0) return;
    scoreForm.classList.remove("hidden");
  }

  function hideScoreForm() {
    if (scoreForm) scoreForm.classList.add("hidden");
  }

  function bindPieceCanvas(cv, slotIdx) {
    cv.addEventListener("pointerdown", (ev) => {
      if (!state?.slots?.[slotIdx]?.active || state.game_over) return;
      ev.preventDefault();
      drag = { slot: slotIdx, cells: state.slots[slotIdx].cells };
      lastSnap = null;
      cv.classList.add("dragging");
      cv.setPointerCapture(ev.pointerId);
    });
    cv.addEventListener("pointermove", (ev) => {
      if (!drag || drag.slot !== slotIdx) return;
      updateGhostFromPointer(ev);
    });
    cv.addEventListener("pointerup", async (ev) => {
      if (!drag || drag.slot !== slotIdx || gameId === null) return;
      cv.classList.remove("dragging");
      try { cv.releasePointerCapture(ev.pointerId); } catch (_) {}
      let snap = lastSnap;
      if (!snap?.ok) {
        const p = boardPos(ev);
        const target = placementTargetFromPointer(drag.cells, p);
        snap = resolvePlacement(drag.cells, target.row, target.col, state?.board);
      }
      ghost = null;
      drag = null;
      lastSnap = null;
      if (!snap.ok) {
        if (statusEl) statusEl.textContent = "此位置無法放置（遊戲可繼續）";
        drawBoard();
        return;
      }
      try {
        const data = await api("/api/block/place", {
          method: "POST",
          body: JSON.stringify({ game_id: gameId, slot: slotIdx, row: snap.row, col: snap.col }),
        });
        if (!data.ok) {
          if (statusEl) statusEl.textContent = "此位置無法放置（遊戲可繼續）";
          drawBoard();
          return;
        }
        applyState(data.state);
      } catch (err) {
        if (statusEl) statusEl.textContent = err.message;
      }
    });
    cv.addEventListener("pointercancel", () => {
      drag = null;
      ghost = null;
      lastSnap = null;
      cv.classList.remove("dragging");
      drawBoard();
    });
  }

  function initDom() {
    boardEl = document.getElementById("puzzle-board");
    boardCtx = boardEl.getContext("2d");
    scoreEl = document.getElementById("puzzle-score");
    statusEl = document.getElementById("puzzle-status");
    wrapEl = document.getElementById("puzzle-wrap");
    leaderboardEl = document.getElementById("puzzle-leaderboard");
    scoreForm = document.getElementById("puzzle-score-form");
    playerNameInput = document.getElementById("puzzle-player-name");
    pieceCanvases = Array.from(document.querySelectorAll(".puzzle-piece-canvas"));
    pieceCanvases.forEach((cv, i) => bindPieceCanvas(cv, i));
    boardEl.addEventListener("pointermove", (ev) => {
      if (!drag) return;
      updateGhostFromPointer(ev);
    });
  }

  async function start() {
    if (!boardEl) initDom();
    hideScoreForm();
    scoreSubmitted = false;
    if (statusEl) statusEl.textContent = "載入中…";

    if (gameId !== null) {
      try {
        await api("/api/block/destroy", {
          method: "POST",
          body: JSON.stringify({ game_id: gameId }),
        });
      } catch (_) {}
    }

    const data = await api("/api/block/new", { method: "POST", body: "{}" });
    gameId = data.game_id;

    const ok = data.state?.slots?.some((s) => s.active && s.cells?.length > 0);
    if (!ok) {
      throw new Error("C 引擎未載入方塊。請執行 build.bat 後重啟 python app.py");
    }

    applyState(data.state);
    loadLeaderboard();
  }

  async function stop() {
    drag = null;
    ghost = null;
    if (gameId !== null) {
      try {
        await api("/api/block/destroy", {
          method: "POST",
          body: JSON.stringify({ game_id: gameId }),
        });
      } catch (_) {}
      gameId = null;
    }
    state = null;
    hideScoreForm();
    pieceCanvases.forEach((cv) => {
      cv.getContext("2d").clearRect(0, 0, cv.width, cv.height);
    });
    if (boardCtx) boardCtx.clearRect(0, 0, boardEl.width, boardEl.height);
    if (scoreEl) scoreEl.textContent = "0";
    if (statusEl) statusEl.textContent = "";
  }

  document.addEventListener("DOMContentLoaded", () => {
    initDom();
    pickTheme(0);
    document.getElementById("puzzle-new")?.addEventListener("click", () => {
      start().catch((e) => { if (statusEl) statusEl.textContent = e.message; });
    });
    scoreForm?.addEventListener("submit", async (ev) => {
      ev.preventDefault();
      if (scoreSubmitted || !state) return;
      const name = (playerNameInput?.value || "").trim() || "Player";
      try {
        await api("/api/puzzle/leaderboard", {
          method: "POST",
          body: JSON.stringify({ name, score: state.score }),
        });
        scoreSubmitted = true;
        hideScoreForm();
        await loadLeaderboard();
      } catch (err) {
        if (statusEl) statusEl.textContent = "送出失敗：" + err.message;
      }
    });
  });

  window.PuzzleGame = { start, stop };
})();
