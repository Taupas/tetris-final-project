const modeSelect = document.getElementById("mode-select");
const classicGame = document.getElementById("classic-game");
const puzzleGame = document.getElementById("puzzle-game");

function showModeSelect() {
  modeSelect.classList.remove("hidden");
  classicGame.classList.add("hidden");
  puzzleGame.classList.add("hidden");
  if (window.ClassicGame) {
    window.ClassicGame.stop();
  }
  if (window.PuzzleGame) {
    window.PuzzleGame.stop();
  }
}

function showClassic() {
  modeSelect.classList.add("hidden");
  classicGame.classList.remove("hidden");
  puzzleGame.classList.add("hidden");
  if (window.PuzzleGame) {
    window.PuzzleGame.stop();
  }
  window.ClassicGame.start().catch((e) => {
    document.getElementById("status").textContent = e.message;
  });
}

function showPuzzle() {
  modeSelect.classList.add("hidden");
  classicGame.classList.add("hidden");
  puzzleGame.classList.remove("hidden");
  if (window.ClassicGame) {
    window.ClassicGame.stop();
  }
  try {
    window.PuzzleGame.start();
  } catch (e) {
    const el = document.getElementById("puzzle-status");
    if (el) el.textContent = e.message || String(e);
  }
}

document.getElementById("btn-mode-classic").addEventListener("click", showClassic);
document.getElementById("btn-mode-puzzle").addEventListener("click", showPuzzle);

document.querySelectorAll("[data-back]").forEach((btn) => {
  btn.addEventListener("click", showModeSelect);
});
