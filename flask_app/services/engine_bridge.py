"""ctypes bridge to C tetris engine."""
from __future__ import annotations

import ctypes
import platform
import sys
from pathlib import Path
from typing import Any

BOARD_WIDTH = 10
BOARD_HEIGHT = 20
BOARD_CELLS = BOARD_WIDTH * BOARD_HEIGHT

BLOCK_GRID = 8
BLOCK_SLOTS = 3
BLOCK_MAX_SHAPE = 6
BLOCK_CELLS = BLOCK_GRID * BLOCK_GRID
BLOCK_SLOT_FLAT = BLOCK_SLOTS * BLOCK_MAX_SHAPE


def _block_can_place(
    board: list[list[int]], cells: list[list[int]], row: int, col: int
) -> bool:
    for dr, dc in cells:
        r, c = row + dr, col + dc
        if r < 0 or r >= BLOCK_GRID or c < 0 or c >= BLOCK_GRID:
            return False
        if board[r][c]:
            return False
    return True


def _block_slot_fits_anywhere(board: list[list[int]], cells: list[list[int]]) -> bool:
    for row in range(BLOCK_GRID):
        for col in range(BLOCK_GRID):
            if _block_can_place(board, cells, row, col):
                return True
    return False


def _block_compute_game_over(
    board: list[list[int]], slots: list[dict[str, Any]]
) -> bool:
    """僅當托盤上所有剩餘方塊都無法放置時才結束（忽略 C DLL 舊版錯誤旗標）。"""
    active = [s for s in slots if s.get("active") and s.get("cells")]
    if not active:
        return False
    return not any(_block_slot_fits_anywhere(board, s["cells"]) for s in active)


class BlockEngineState(ctypes.Structure):
    _fields_ = [
        ("board", ctypes.c_int * BLOCK_CELLS),
        ("slot_len", ctypes.c_int * BLOCK_SLOTS),
        ("slot_active", ctypes.c_int * BLOCK_SLOTS),
        ("slot_dr", ctypes.c_int * BLOCK_SLOT_FLAT),
        ("slot_dc", ctypes.c_int * BLOCK_SLOT_FLAT),
        ("score", ctypes.c_int),
        ("theme", ctypes.c_int),
        ("theme_changed", ctypes.c_int),
        ("game_over", ctypes.c_int),
    ]


class EngineState(ctypes.Structure):
    _fields_ = [
        ("board", ctypes.c_int * BOARD_CELLS),
        ("active_rows", ctypes.c_int * 4),
        ("active_cols", ctypes.c_int * 4),
        ("active_count", ctypes.c_int),
        ("active_type", ctypes.c_int),
        ("next_type", ctypes.c_int),
        ("score", ctypes.c_int),
        ("lines", ctypes.c_int),
        ("level", ctypes.c_int),
        ("game_over", ctypes.c_int),
    ]


def _lib_path() -> Path:
    root = Path(__file__).resolve().parents[2]
    lib_dir = root / "lib"
    if sys.platform == "win32":
        return lib_dir / "tetris_engine.dll"
    if platform.system() == "Darwin":
        return lib_dir / "libtetris_engine.dylib"
    return lib_dir / "libtetris_engine.so"


class TetrisEngine:
    def __init__(self) -> None:
        path = _lib_path()
        if not path.exists():
            raise FileNotFoundError(
                f"找不到 C 引擎函式庫：{path}\n請在 tetris 目錄執行 build.bat 或 make"
            )
        self._lib = ctypes.CDLL(str(path))
        self._bind()
        self._state = EngineState()
        self._block_state = BlockEngineState()

    def _bind(self) -> None:
        lib = self._lib
        lib.engine_create.argtypes = [ctypes.c_uint]
        lib.engine_create.restype = ctypes.c_int
        lib.engine_destroy.argtypes = [ctypes.c_int]
        lib.engine_destroy.restype = None
        for name in (
            "engine_tick",
            "engine_move_left",
            "engine_move_right",
            "engine_soft_drop",
            "engine_rotate_cw",
            "engine_rotate_ccw",
            "engine_hard_drop",
        ):
            fn = getattr(lib, name)
            fn.argtypes = [ctypes.c_int]
            fn.restype = ctypes.c_int
        lib.engine_get_state.argtypes = [ctypes.c_int, ctypes.POINTER(EngineState)]
        lib.engine_get_state.restype = ctypes.c_int
        lib.block_create.argtypes = [ctypes.c_uint]
        lib.block_create.restype = ctypes.c_int
        lib.block_destroy.argtypes = [ctypes.c_int]
        lib.block_destroy.restype = None
        lib.block_place.argtypes = [
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int,
        ]
        lib.block_place.restype = ctypes.c_int
        lib.block_get_state.argtypes = [ctypes.c_int, ctypes.POINTER(BlockEngineState)]
        lib.block_get_state.restype = ctypes.c_int

    def create(self, seed: int = 0) -> int:
        game_id = self._lib.engine_create(ctypes.c_uint(seed & 0xFFFFFFFF))
        if game_id < 0:
            raise RuntimeError("無法建立遊戲（已達上限）")
        return game_id

    def destroy(self, game_id: int) -> None:
        self._lib.engine_destroy(game_id)

    def tick(self, game_id: int) -> bool:
        return bool(self._lib.engine_tick(game_id))

    def move_left(self, game_id: int) -> bool:
        return bool(self._lib.engine_move_left(game_id))

    def move_right(self, game_id: int) -> bool:
        return bool(self._lib.engine_move_right(game_id))

    def soft_drop(self, game_id: int) -> bool:
        return bool(self._lib.engine_soft_drop(game_id))

    def rotate_cw(self, game_id: int) -> bool:
        return bool(self._lib.engine_rotate_cw(game_id))

    def rotate_ccw(self, game_id: int) -> bool:
        return bool(self._lib.engine_rotate_ccw(game_id))

    def hard_drop(self, game_id: int) -> bool:
        return bool(self._lib.engine_hard_drop(game_id))

    def get_state(self, game_id: int) -> dict[str, Any]:
        ok = self._lib.engine_get_state(game_id, ctypes.byref(self._state))
        if not ok:
            raise ValueError(f"無效的 game_id: {game_id}")
        s = self._state
        grid = []
        for r in range(BOARD_HEIGHT):
            row = []
            for c in range(BOARD_WIDTH):
                val = s.board[r * BOARD_WIDTH + c]
                row.append(val)
            grid.append(row)
        for i in range(s.active_count):
            ar, ac = s.active_rows[i], s.active_cols[i]
            if 0 <= ar < BOARD_HEIGHT and 0 <= ac < BOARD_WIDTH:
                grid[ar][ac] = s.active_type + 1
        return {
            "board": grid,
            "next_type": s.next_type,
            "score": s.score,
            "lines": s.lines,
            "level": s.level,
            "game_over": bool(s.game_over),
        }

    def block_create(self, seed: int = 0) -> int:
        game_id = self._lib.block_create(ctypes.c_uint(seed & 0xFFFFFFFF))
        if game_id < 0:
            raise RuntimeError("無法建立方塊拼圖遊戲")
        return game_id

    def block_destroy(self, game_id: int) -> None:
        self._lib.block_destroy(game_id)

    def block_place(self, game_id: int, slot: int, row: int, col: int) -> bool:
        return bool(self._lib.block_place(game_id, slot, row, col))

    def block_get_state(self, game_id: int) -> dict[str, Any]:
        ok = self._lib.block_get_state(game_id, ctypes.byref(self._block_state))
        if not ok:
            raise ValueError(f"無效的 block game_id: {game_id}")
        s = self._block_state
        grid = []
        for r in range(BLOCK_GRID):
            row = []
            for c in range(BLOCK_GRID):
                row.append(int(s.board[r * BLOCK_GRID + c]))
            grid.append(row)
        slots = []
        for i in range(BLOCK_SLOTS):
            if s.slot_active[i]:
                n = int(s.slot_len[i])
                base = i * BLOCK_MAX_SHAPE
                cells = [
                    [int(s.slot_dr[base + j]), int(s.slot_dc[base + j])]
                    for j in range(n)
                ]
                slots.append({"active": True, "cells": cells})
            else:
                slots.append({"active": False, "cells": []})
        game_over = _block_compute_game_over(grid, slots)
        return {
            "board": grid,
            "slots": slots,
            "score": int(s.score),
            "theme": int(s.theme),
            "theme_changed": bool(s.theme_changed),
            "game_over": game_over,
        }


_engine: TetrisEngine | None = None


def get_engine() -> TetrisEngine:
    global _engine
    if _engine is None:
        _engine = TetrisEngine()
    return _engine
