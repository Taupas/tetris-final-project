@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

echo ========================================
echo  Tetris 專題 - 推送到 GitHub
echo ========================================
echo.

where git >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [錯誤] 找不到 git。請先安裝 Git for Windows：
    echo   https://git-scm.com/download/win
    echo.
    echo 安裝時請勾選「Add Git to PATH」，安裝完關閉並重開終端機後再執行本腳本。
    echo.
    echo 若暫時無法安裝 Git，請改用網頁上傳（見 submission\01_GITHUB_REPOSITORY.md）
    pause
    exit /b 1
)

set /p REPO_URL="請貼上 GitHub 倉庫網址（例 https://github.com/你的帳號/tetris-final-project.git）: "
if "%REPO_URL%"=="" (
    echo [取消] 未輸入網址
    pause
    exit /b 1
)

if not exist ".git" (
    echo [1/4] git init ...
    git init
)

echo [2/4] git add ...
git add .

echo [3/4] git commit ...
git commit -m "feat: 俄羅斯方塊 Flask + C 引擎期末專題" 2>nul
if %ERRORLEVEL% neq 0 (
    echo （若顯示 nothing to commit 表示已提交過，繼續推送）
)

echo [4/4] 設定遠端並推送 ...
git branch -M main 2>nul
git remote remove origin 2>nul
git remote add origin "%REPO_URL%"
git push -u origin main

if %ERRORLEVEL% neq 0 (
    echo.
    echo [失敗] 推送失敗。常見原因：
    echo   1. 尚未在 GitHub 網站建立同名空倉庫
    echo   2. 未登入 GitHub（需先在瀏覽器登入，或使用 GitHub Desktop）
    echo   3. 網址打錯或沒有權限
    echo.
    pause
    exit /b 1
)

echo.
echo [完成] 已推送到：%REPO_URL%
echo 請把此連結填入 submission\01_GITHUB_REPOSITORY.md
pause
