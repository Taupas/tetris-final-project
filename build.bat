@echo off
setlocal
cd /d "%~dp0"

if not exist "lib" mkdir lib

where gcc >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [錯誤] 找不到 gcc，請安裝 MinGW-w64 並加入 PATH
    exit /b 1
)

gcc -shared -O2 -I"c_engine\include" ^
    c_engine\src\queue.c ^
    c_engine\src\board.c ^
    c_engine\src\tetromino.c ^
    c_engine\src\physics.c ^
    c_engine\src\score.c ^
    c_engine\src\engine_api.c ^
    c_engine\src\block_puzzle_api.c ^
    -o lib\tetris_engine.dll

if %ERRORLEVEL% neq 0 (
    echo [錯誤] 編譯失敗
    exit /b 1
)

echo [完成] lib\tetris_engine.dll
exit /b 0
