"""Flask frontend for Tetris (C engine backend)."""
from __future__ import annotations

import json
import time
from pathlib import Path

from flask import Flask, jsonify, redirect, render_template, request

from services.engine_bridge import get_engine

APP_DIR = Path(__file__).resolve().parent
LEADERBOARD_PATH = APP_DIR / "data" / "leaderboard.json"
PUZZLE_LEADERBOARD_PATH = APP_DIR / "data" / "puzzle_leaderboard.json"

# 遊戲網址前綴：http://127.0.0.1:5000/tetris
GAME_PREFIX = "/tetris"

app = Flask(__name__, template_folder="templates", static_folder="static")


def _load_scores(path: Path) -> list[dict]:
    if not path.exists():
        return []
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError):
        return []


def _save_scores(path: Path, rows: list[dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    rows.sort(key=lambda x: x.get("score", 0), reverse=True)
    path.write_text(
        json.dumps(rows[:10], ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


def _load_leaderboard() -> list[dict]:
    return _load_scores(LEADERBOARD_PATH)


def _save_leaderboard(rows: list[dict]) -> None:
    _save_scores(LEADERBOARD_PATH, rows)


@app.route("/")
def root():
    return redirect(f"{GAME_PREFIX}/")


@app.route(GAME_PREFIX)
@app.route(f"{GAME_PREFIX}/")
def index():
    return render_template("index.html", app_base=GAME_PREFIX)


@app.post(f"{GAME_PREFIX}/api/new")
def api_new():
    seed = int(request.json.get("seed", 0) if request.is_json else 0)
    if seed == 0:
        seed = int(time.time()) & 0xFFFFFFFF
    engine = get_engine()
    game_id = engine.create(seed)
    return jsonify({"game_id": game_id, "state": engine.get_state(game_id)})


@app.post(f"{GAME_PREFIX}/api/input")
def api_input():
    data = request.get_json(force=True)
    game_id = int(data["game_id"])
    action = data.get("action", "")
    engine = get_engine()
    handlers = {
        "left": engine.move_left,
        "right": engine.move_right,
        "down": engine.soft_drop,
        "rotate_cw": engine.rotate_cw,
        "rotate_ccw": engine.rotate_ccw,
        "hard_drop": engine.hard_drop,
    }
    fn = handlers.get(action)
    if fn:
        fn(game_id)
    return jsonify({"ok": True, "state": engine.get_state(game_id)})


@app.post(f"{GAME_PREFIX}/api/tick")
def api_tick():
    data = request.get_json(force=True)
    game_id = int(data["game_id"])
    engine = get_engine()
    engine.tick(game_id)
    return jsonify({"state": engine.get_state(game_id)})


@app.get(f"{GAME_PREFIX}/api/state")
def api_state():
    game_id = int(request.args.get("game_id", -1))
    engine = get_engine()
    return jsonify({"state": engine.get_state(game_id)})


@app.post(f"{GAME_PREFIX}/api/block/new")
def api_block_new():
    seed = int(request.json.get("seed", 0) if request.is_json else 0)
    if seed == 0:
        seed = int(time.time()) & 0xFFFFFFFF
    engine = get_engine()
    game_id = engine.block_create(seed)
    return jsonify({"game_id": game_id, "state": engine.block_get_state(game_id)})


@app.post(f"{GAME_PREFIX}/api/block/place")
def api_block_place():
    data = request.get_json(force=True)
    game_id = int(data["game_id"])
    slot = int(data["slot"])
    row = int(data["row"])
    col = int(data["col"])
    engine = get_engine()
    ok = engine.block_place(game_id, slot, row, col)
    return jsonify({"ok": ok, "state": engine.block_get_state(game_id)})


@app.post(f"{GAME_PREFIX}/api/block/destroy")
def api_block_destroy():
    data = request.get_json(force=True)
    game_id = int(data["game_id"])
    get_engine().block_destroy(game_id)
    return jsonify({"ok": True})


@app.post(f"{GAME_PREFIX}/api/destroy")
def api_destroy():
    data = request.get_json(force=True)
    game_id = int(data["game_id"])
    get_engine().destroy(game_id)
    return jsonify({"ok": True})


@app.get(f"{GAME_PREFIX}/api/leaderboard")
def api_leaderboard():
    return jsonify({"entries": _load_leaderboard()})


@app.post(f"{GAME_PREFIX}/api/leaderboard")
def api_leaderboard_post():
    data = request.get_json(force=True)
    name = (data.get("name") or "Player")[:20]
    score = int(data.get("score", 0))
    rows = _load_leaderboard()
    rows.append({"name": name, "score": score, "ts": int(time.time())})
    _save_leaderboard(rows)
    return jsonify({"entries": _load_leaderboard()})


@app.get(f"{GAME_PREFIX}/api/puzzle/leaderboard")
def api_puzzle_leaderboard():
    return jsonify({"entries": _load_scores(PUZZLE_LEADERBOARD_PATH)})


@app.post(f"{GAME_PREFIX}/api/puzzle/leaderboard")
def api_puzzle_leaderboard_post():
    data = request.get_json(force=True)
    name = (data.get("name") or "Player")[:20]
    score = int(data.get("score", 0))
    rows = _load_scores(PUZZLE_LEADERBOARD_PATH)
    rows.append({"name": name, "score": score, "ts": int(time.time())})
    _save_scores(PUZZLE_LEADERBOARD_PATH, rows)
    return jsonify({"entries": _load_scores(PUZZLE_LEADERBOARD_PATH)})


if __name__ == "__main__":
    get_engine()
    app.run(debug=True, host="0.0.0.0", port=5000)
