#!/usr/bin/env python3
"""Validate headless LVGL 480x480 round-screen images and emit portable PNGs.

Run after: motonav-lvgl-simulator --capture-dir /tmp/motonav-capture --smoke-test
Pure Python standard library; works in GitHub Actions without actual ESP32.
"""
from __future__ import annotations
import binascii
import html
import pathlib
import struct
import sys
import zlib

SCENES = (
    "home", "weather", "music", "companion", "stocks", "settings",
    "nav-straight", "nav-left", "nav-right", "nav-near",
    "nav-overspeed", "nav-uturn", "nav-arrive", "home-offline",
)
SIZE = 480

def ppm(path: pathlib.Path) -> bytes:
    raw = path.read_bytes()
    prefix = f"P6\n{SIZE} {SIZE}\n255\n".encode()
    if not raw.startswith(prefix):
        raise AssertionError(f"{path} is not a {SIZE}x{SIZE} raw PPM")
    image = raw[len(prefix):]
    if len(image) != SIZE * SIZE * 3:
        raise AssertionError(f"{path}: bad size {len(image)}")
    # 480 physical panel with 466 logical image must have black corners.
    for x, y in ((0, 0), (479, 0), (0, 479), (479, 479),
                 (240, 0), (0, 240), (479, 240), (240, 479)):
        off = (y * SIZE + x) * 3
        if image[off:off+3] != b"\x00\x00\x00":
            raise AssertionError(f"{path}: circular mask leaked at {x},{y}")
    visible = sum(1 for i in range(0, len(image), 3)
                  if image[i] | image[i+1] | image[i+2])
    if visible < 9000:
        raise AssertionError(f"{path}: suspiciously blank frame ({visible} pixels)")
    return image

def chunk(tag: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + tag + data + struct.pack(
        ">I", binascii.crc32(tag + data) & 0xFFFFFFFF)

def png(path: pathlib.Path, rgb: bytes) -> None:
    stride = SIZE * 3
    rows = b"".join(b"\x00" + rgb[y*stride:(y+1)*stride] for y in range(SIZE))
    ihdr = struct.pack(">IIBBBBB", SIZE, SIZE, 8, 2, 0, 0, 0)
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) +
                     chunk(b"IDAT", zlib.compress(rows, 6)) + chunk(b"IEND", b""))

def main(folder: pathlib.Path) -> None:
    pictures = {scene: ppm(folder / (scene + ".ppm")) for scene in SCENES}
    contrasts = (
        ("nav-left", "nav-right"),
        ("nav-near", "nav-right"),
        ("nav-overspeed", "nav-right"),
        ("nav-uturn", "nav-straight"),
        ("nav-arrive", "nav-straight"),
        ("nav-arrive", "nav-uturn"),
        ("home", "home-offline"),
        ("weather", "home"),
    )
    for a, b in contrasts:
        if pictures[a] == pictures[b]:
            raise AssertionError(f"Visual state did not change between {a} and {b}")
    for scene, pixels in pictures.items():
        png(folder / (scene + ".png"), pixels)
    cards = "\n".join(
        f'<figure><img alt="{html.escape(scene)}" src="{scene}.png">'
        f'<figcaption>{html.escape(scene)}</figcaption></figure>' for scene in SCENES
    )
    (folder / "index.html").write_text(
        '<!doctype html><html lang="en"><meta charset="utf-8">'
        '<title>MotoNav LVGL boardless visual verification</title>'
        '<style>body{font-family:system-ui;background:#121b24;color:#e6f3f7;'
        'padding:22px}main{display:grid;grid-template-columns:repeat('
        'auto-fill,minmax(210px,1fr));gap:18px}figure{margin:0}'
        'img{width:100%;border-radius:50%;background:#000}'
        'figcaption{text-align:center;margin-top:8px}</style>'
        '<h1>MotoNav LVGL — mock scenes (not hardware screenshots)</h1>'
        '<p>480×480 simulated round panel. Device rotations, BLE and real map '
        'data require physical verification.</p><main>' + cards + '</main></html>',
        encoding="utf-8",
    )
    print(f"Verified {len(pictures)} simulated panels and {len(contrasts)} "
          f"state contrasts; wrote PNGs and index.html to {folder}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: verify_captures.py /path/to/capture-dir")
    main(pathlib.Path(sys.argv[1]))
