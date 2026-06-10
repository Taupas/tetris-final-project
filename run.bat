@echo off
setlocal
cd /d "%~dp0"

if not exist "lib\tetris_engine.dll" (
    echo 正在編譯 C 引擎...
    call build.bat
    if errorlevel 1 exit /b 1
)

cd flask_app
python -m pip install -r ..\requirements.txt -q
python app.py
