#!/usr/bin/env python3
"""Convert a local photo into the exact MotoNav 240x240 RGB565 BDG1 packet.

Example:
  python3 -m pip install Pillow
  python3 tools/lvgl-simulator/make_badge.py photo.jpg /tmp/photo.bdg
  ./build/lvgl-simulator/motonav-lvgl-simulator --badge-file /tmp/photo.bdg

The picture stays on your own computer. No network or ESP32 is required.
"""
import argparse
import struct
import zlib
from pathlib import Path

SIZE = 240
HEADER = b"BDG1"


def build_badge(src: Path) -> bytes:
    try:
        from PIL import Image, ImageOps
    except ImportError as exc:
        raise SystemExit("Requires Pillow: python3 -m pip install Pillow") from exc
    with Image.open(src) as original:
        # Normalize phone camera orientation before cropping.
        image = ImageOps.exif_transpose(original).convert("RGB")
        width, height = image.size
        if width <= 0 or height <= 0:
            raise ValueError("Invalid input image dimensions")
        side = min(width, height)
        left, top = (width - side) // 2, (height - side) // 2
        image = image.crop((left, top, left + side, top + side))
        image = image.resize((SIZE, SIZE), Image.Resampling.LANCZOS)
        pixels = image.tobytes()
    rgb565 = bytearray(SIZE * SIZE * 2)
    for i in range(SIZE * SIZE):
        red, green, blue = pixels[i * 3 : i * 3 + 3]
        value = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)
        struct.pack_into("<H", rgb565, 2 * i, value)
    header = HEADER + struct.pack("<HHI", SIZE, SIZE, zlib.crc32(rgb565))
    return header + rgb565


def main():
    parser = argparse.ArgumentParser(description="Prepare a MotoNav BDG1 photo preview")
    parser.add_argument("photo", type=Path, help="Input JPG, PNG, WEBP etc.")
    parser.add_argument("output", type=Path, help="Output .bdg file")
    args = parser.parse_args()
    blob = build_badge(args.photo)
    args.output.write_bytes(blob)
    print(f"Wrote {args.output} ({len(blob)} bytes, CRC32-verified by simulator)")


if __name__ == "__main__":
    main()
