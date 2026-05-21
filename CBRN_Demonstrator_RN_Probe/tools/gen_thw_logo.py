"""
Lädt das THW-SVG-Logo von Wikimedia, rastert es auf TARGET x TARGET Pixel
1-Bit-Bitmap (schwarz/weiß) und schreibt es als C++-Header
include/thw_logo.h - geeignet für Adafruit_GFX::drawBitmap().

Re-Ausführung:
  tools/.venv/Scripts/python.exe tools/gen_thw_logo.py
"""

from __future__ import annotations

import io
import sys
import urllib.request
from pathlib import Path

from PIL import Image
from reportlab.graphics import renderPM
from svglib.svglib import svg2rlg

SVG_URL = "https://upload.wikimedia.org/wikipedia/commons/f/f9/THW.svg"
TARGET = 130                                            # logo wird TARGET x TARGET Pixel
THRESHOLD = 200                                         # Pixel < THRESHOLD -> schwarz (1)
OUT_HEADER = Path(__file__).resolve().parent.parent / "include" / "thw_logo.h"
SVG_CACHE = Path(__file__).resolve().parent / "_thw.svg"


def fetch_svg() -> Path:
    if not SVG_CACHE.exists():
        print(f"download {SVG_URL}")
        # Wikimedia blockt den Default-Python-UA, deshalb explizit ein
        # Browser-UA setzen.
        req = urllib.request.Request(
            SVG_URL,
            headers={"User-Agent": "Mozilla/5.0 (gen_thw_logo.py)"},
        )
        data = urllib.request.urlopen(req, timeout=30).read()
        SVG_CACHE.write_bytes(data)
    return SVG_CACHE


def render_to_image(svg_path: Path, size: int) -> Image.Image:
    # Native rendern (800x800 laut SVG), dann mit Lanczos auf size runterskalieren.
    drawing = svg2rlg(str(svg_path))
    png_bytes = renderPM.drawToString(drawing, fmt="PNG")
    img = Image.open(io.BytesIO(png_bytes)).convert("RGBA")
    # Auf weiss komponieren - sonst wird transparent zu schwarz.
    bg = Image.new("RGBA", img.size, (255, 255, 255, 255))
    img = Image.alpha_composite(bg, img).convert("L")
    if img.size != (size, size):
        img = img.resize((size, size), Image.LANCZOS)
    return img


def to_bitmap_bytes(img: Image.Image) -> tuple[bytes, int, int]:
    # 1-Bit-Konvertierung mit harter Schwelle (kein Dithering -
    # E-Paper sieht ohne Graustufen besser aus).
    bw = img.point(lambda p: 0 if p < THRESHOLD else 255, mode="1")
    w, h = bw.size
    row_bytes = (w + 7) // 8
    px = bw.load()
    out = bytearray()
    for y in range(h):
        for bx in range(row_bytes):
            b = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < w and px[x, y] == 0:   # 0 = schwarz in Modus '1'
                    b |= 1 << (7 - bit)
            out.append(b)
    return bytes(out), w, h


def write_header(data: bytes, w: int, h: int, out: Path) -> None:
    lines: list[str] = []
    lines.append("// Auto-generiert aus THW.svg - nicht editieren.")
    lines.append("// Neu erzeugen: tools/.venv/Scripts/python.exe tools/gen_thw_logo.py")
    lines.append("#pragma once")
    lines.append("#include <Arduino.h>")
    lines.append("")
    lines.append(f"constexpr int THW_LOGO_W = {w};")
    lines.append(f"constexpr int THW_LOGO_H = {h};")
    lines.append("")
    lines.append("constexpr uint8_t THW_LOGO[] PROGMEM = {")
    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        lines.append("  " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    lines.append("};")
    lines.append("")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines), encoding="utf-8")
    print(f"wrote {out} ({len(data)} bytes, {w}x{h})")


def main() -> int:
    svg = fetch_svg()
    img = render_to_image(svg, TARGET)
    data, w, h = to_bitmap_bytes(img)
    write_header(data, w, h, OUT_HEADER)
    return 0


if __name__ == "__main__":
    sys.exit(main())
