const COLORS = [
  null,
  "#00f0f0", /* I */
  "#f0f000", /* O */
  "#a000f0", /* T */
  "#00f000", /* S */
  "#f00000", /* Z */
  "#0000f0", /* J */
  "#f0a000", /* L */
];

const PREVIEW_SHAPES = [
  [[1, 1, 1, 1]],
  [[1, 1], [1, 1]],
  [[0, 1, 0], [1, 1, 1]],
  [[0, 1, 1], [1, 1, 0]],
  [[1, 1, 0], [0, 1, 1]],
  [[1, 0, 0], [1, 1, 1]],
  [[0, 0, 1], [1, 1, 1]],
];

const boardCanvas = document.getElementById("board-canvas");
const nextCanvas = document.getElementById("next-canvas");
const boardCtx = boardCanvas.getContext("2d");
const nextCtx = nextCanvas.getContext("2d");

const scoreEl = document.getElementById("score");
const linesEl = document.getElementById("lines");
const levelEl = document.getElementById("level");
const statusEl = document.getElementById("status");
const leaderboardEl = document.getElementById("leaderboard");
const scoreForm = document.getElementById("score-form");
const playerNameInput = document.getElementById("player-name");

const COLS = 10;
const ROWS = 20;
const CELL = boardCanvas.width / COLS;

let gameId = null;
let paused = false;
let tickTimer = null;
let lastScore = 0;

function gravityMs(level) {
  return Math.max(100, 800 - (level - 1) * 50);
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
  if (!res.ok) {
    const text = await res.text();
    throw new Error(text || res.statusText);
  }
  return res.json();
}

function drawCell(ctx, x, y, size, color) {
  if (!color) return;
  ctx.fillStyle = color;
  ctx.fillRect(x * size + 1, y * size + 1, size - 2, size - 2);
}

function drawBoard(state) {
  boardCtx.clearRect(0, 0, boardCanvas.width, boardCanvas.height);
  for (let r = 0; r < ROWS; r++) {
    for (let c = 0; c < COLS; c++) {
      const v = state.board[r][c];
      drawCell(boardCtx, c, r, CELL, COLORS[v] || null);
    }
  }
}

function drawNext(type) {
  nextCtx.clearRect(0, 0, nextCanvas.width, nextCanvas.height);
  const shape = PREVIEW_SHAPES[type];
  if (!shape) return;
  const size = 24;
  const offX = (nextCanvas.width - shape[0].length * size) / 2;
  const offY = (nextCanvas.height - shape.length * size) / 2;
  const color = COLORS[type + 1];
  for (let r = 0; r < shape.length; r++) {
    for (let c = 0; c < shape[r].length; c++) {
      if (shape[r][c]) {
        nextCtx.fillStyle = color;
        nextCtx.fillRect(offX + c * size, offY + r * size, size - 2, size - 2);
      }
    }
  }
}

function updateHud(state) {
  scoreEl.textContent = state.score;
  linesEl.textContent = state.lines;
  levelEl.textContent = state.level;
  drawBoard(state);
  drawNext(state.next_type);
  lastScore = state.score;

  if (state.game_over) {
    statusEl.textContent = "遊戲結束";
    stopTick();
    scoreForm.classList.remove("hidden");
  } else {
    statusEl.textContent = paused ? "已暫停" : "";
  }
}

function stopTick() {
  if (tickTimer) {
    clearInterval(tickTimer);
    tickTimer = null;
  }
}

function startTick(level) {
  stopTick();
  tickTimer = setInterval(async () => {
    if (paused || gameId === null) return;
    try {
      const data = await api("/api/tick", {
        method: "POST",
        body: JSON.stringify({ game_id: gameId }),
      });
      updateHud(data.state);
      if (data.state.game_over) return;
      if (data.state.level !== level) {
        startTick(data.state.level);
      }
    } catch (e) {
      statusEl.textContent = "連線錯誤";
      stopTick();
    }
  }, gravityMs(level));
}

async function newGame() {
  stopTick();
  paused = false;
  scoreForm.classList.add("hidden");
  if (gameId !== null) {
    try {
      await api("/api/destroy", {
        method: "POST",
        body: JSON.stringify({ game_id: gameId }),
      });
    } catch (_) {
      /* ignore */
    }
  }
  const data = await api("/api/new", { method: "POST", body: "{}" });
  gameId = data.game_id;
  updateHud(data.state);
  startTick(data.state.level);
}

async function sendInput(action) {
  if (gameId === null || paused) return;
  const data = await api("/api/input", {
    method: "POST",
    body: JSON.stringify({ game_id: gameId, action }),
  });
  updateHud(data.state);
}

async function loadLeaderboard() {
  const data = await api("/api/leaderboard");
  leaderboardEl.innerHTML = "";
  data.entries.forEach((e, i) => {
    const li = document.createElement("li");
    li.textContent = `${i + 1}. ${e.name} — ${e.score}`;
    leaderboardEl.appendChild(li);
  });
}

document.getElementById("btn-new").addEventListener("click", () => {
  newGame().catch((e) => {
    statusEl.textContent = e.message;
  });
});

document.getElementById("btn-pause").addEventListener("click", () => {
  if (gameId === null) return;
  paused = !paused;
  statusEl.textContent = paused ? "已暫停" : "";
});

scoreForm.addEventListener("submit", async (ev) => {
  ev.preventDefault();
  const name = playerNameInput.value.trim() || "Player";
  await api("/api/leaderboard", {
    method: "POST",
    body: JSON.stringify({ name, score: lastScore }),
  });
  scoreForm.classList.add("hidden");
  loadLeaderboard();
});

document.addEventListener("keydown", (ev) => {
  if (gameId === null) return;
  const map = {
    ArrowLeft: "left",
    ArrowRight: "right",
    ArrowDown: "down",
    ArrowUp: "rotate_cw",
    " ": "hard_drop",
  };
  if (ev.key === "p" || ev.key === "P") {
    paused = !paused;
    statusEl.textContent = paused ? "已暫停" : "";
    return;
  }
  const action = map[ev.key];
  if (!action) return;
  ev.preventDefault();
  sendInput(action).catch((e) => {
    statusEl.textContent = e.message;
  });
});

async function stopClassic() {
  stopTick();
  if (gameId !== null) {
    try {
      await api("/api/destroy", {
        method: "POST",
        body: JSON.stringify({ game_id: gameId }),
      });
    } catch (_) {
      /* ignore */
    }
    gameId = null;
  }
  paused = false;
  scoreForm.classList.add("hidden");
}

window.ClassicGame = {
  start() {
    loadLeaderboard().catch(() => {});
    return newGame();
  },
  stop: stopClassic,
};
