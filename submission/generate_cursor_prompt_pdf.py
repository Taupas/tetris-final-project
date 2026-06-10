# -*- coding: utf-8 -*-
"""Generate Cursor Prompt.pdf for Tetris final project."""
from pathlib import Path

from reportlab.lib.pagesizes import A4
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas

ROOT = Path(__file__).resolve().parent
TXT = ROOT / "Cursor_Prompt_內容.txt"
OUT_PROJECT = ROOT / "Cursor Prompt.pdf"
OUT_DOWNLOADS = Path(r"C:\Users\USER\Downloads\Cursor Prompt.pdf")

FONT_CANDIDATES = [
    Path(r"C:\Windows\Fonts\msyh.ttc"),
    Path(r"C:\Windows\Fonts\msyhbd.ttc"),
    Path(r"C:\Windows\Fonts\mingliu.ttc"),
    Path(r"C:\Windows\Fonts\simsun.ttc"),
]


def pick_font() -> Path:
    for p in FONT_CANDIDATES:
        if p.exists():
            return p
    raise FileNotFoundError("找不到支援中文的系統字型")


def wrap_line(text: str, font_name: str, font_size: int, max_width: float) -> list[str]:
    if not text.strip():
        return [""]
    lines: list[str] = []
    current = ""
    for ch in text:
        trial = current + ch
        if pdfmetrics.stringWidth(trial, font_name, font_size) <= max_width:
            current = trial
        else:
            if current:
                lines.append(current)
            current = ch
    if current:
        lines.append(current)
    return lines


def build_pdf(dest: Path) -> None:
    font_path = pick_font()
    font_name = "ProjectCJK"
    pdfmetrics.registerFont(TTFont(font_name, str(font_path)))

    text = TXT.read_text(encoding="utf-8")
    page_w, page_h = A4
    margin_l, margin_r, margin_t, margin_b = 50, 50, 50, 50
    max_width = page_w - margin_l - margin_r
    font_size = 11
    line_height = 16

    c = canvas.Canvas(str(dest), pagesize=A4)
    c.setFont(font_name, font_size)

    y = page_h - margin_t
    for raw_line in text.splitlines():
        wrapped = wrap_line(raw_line, font_name, font_size, max_width)
        for line in wrapped:
            if y < margin_b:
                c.showPage()
                c.setFont(font_name, font_size)
                y = page_h - margin_t
            c.drawString(margin_l, y, line)
            y -= line_height

    c.save()


if __name__ == "__main__":
    build_pdf(OUT_PROJECT)
    build_pdf(OUT_DOWNLOADS)
    print(f"已產生：{OUT_PROJECT}")
    print(f"已覆寫：{OUT_DOWNLOADS}")
