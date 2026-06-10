@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set REPO_NAME=tetris-final-project

where git >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [錯誤] 找不到 git，請安裝 Git for Windows 後重試
    pause
    exit /b 1
)

where gh >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [錯誤] 找不到 gh，請安裝 GitHub CLI: winget install GitHub.cli
    pause
    exit /b 1
)

gh auth status >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [提示] 尚未登入 GitHub，即將啟動登入流程...
    echo 請在瀏覽器完成授權後，再執行本腳本一次。
    gh auth login -h github.com -p https -w
    if errorlevel 1 exit /b 1
)

if not exist ".git" (
    echo [1/4] git init ...
    git init
    git add .
    git commit -m "feat: 俄羅斯方塊 Flask + C 引擎期末專題"
)

git branch -M main 2>nul

echo [2/4] 建立 GitHub 倉庫 %REPO_NAME% ...
gh repo view %REPO_NAME% >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo 倉庫已存在，直接推送...
    git remote remove origin 2>nul
    for /f "delims=" %%i in ('gh repo view %REPO_NAME% --json url -q .url') do set REPO_URL=%%i
    git remote add origin !REPO_URL!.git
) else (
    gh repo create %REPO_NAME% --public --source=. --remote=origin --description "1142 計算機程式語言期末專題 — Flask x C 俄羅斯方塊"
    if errorlevel 1 exit /b 1
)

echo [3/4] git push ...
git push -u origin main
if errorlevel 1 exit /b 1

echo.
echo [完成] 倉庫網址：
gh repo view %REPO_NAME% --json url -q .url
echo.
pause
