import os
import subprocess
import webbrowser
import time

base_dir = os.path.dirname(os.path.abspath(__file__))

subprocess.call(os.path.join(base_dir, "build.bat"), shell=True)

flask_dir = os.path.join(base_dir, "flask_app")

proc = subprocess.Popen(
    ["python", "app.py"],
    cwd=flask_dir
)

time.sleep(2)

webbrowser.open("http://127.0.0.1:5000/tetris")

proc.wait()
