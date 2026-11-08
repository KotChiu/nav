#!/usr/bin/env python3
"""Build the shared iOS/Android firmware; never flash a device."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parent
FQBN = 'esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,CDCOnBoot=cdc'
BOARDS = {
    '175': {
        'label': '1.75',
        'name': 'Waveshare ESP32-S3-Touch-AMOLED-1.75',
    },
    '21': {
        'label': '2.1',
        'name': 'Waveshare ESP32-S3-Touch-LCD-2.1',
    },
}

def ensure_library(cli, name, version):
    listed = subprocess.run([cli, 'lib', 'list'], check=True, capture_output=True, text=True)
    for line in listed.stdout.splitlines():
        fields = line.split()
        if len(fields) >= 2 and fields[0] == name and fields[1] == version:
            return
    subprocess.run([cli, 'lib', 'install', f'{name}@{version}'], check=True)


def package(output, dest, version, board):
    app = (output / 'MotoNav.ino.bin').read_bytes()
    merged = (output / 'MotoNav.ino.merged.bin').read_bytes()
    if not app or app[0] != 0xE9 or merged[0x10000:0x10000 + len(app)] != app:
        raise ValueError('Invalid ESP32 image or unexpected application offset')
    dest.mkdir(parents=True, exist_ok=True)
    board = BOARDS[board]
    label = f'MotoNav_universal_{board["label"]}_v{version}_panel0'
    entries = []
    for suffix, data in [('app_0x10000', app), ('full_0x0', merged[:0x10000 + len(app)])]:
        name = f'{label}_{suffix}.bin'
        (dest / name).write_bytes(data)
        entries.append(dict(file=name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest()))
    manifest = dict(firmware=version, platform="universal", board=board['name'],
                    esp32Core='3.3.10', flashSize='16MB', partition='app3M_fat9M_16MB',
                    verifiedApplicationOffset='0x10000', deviceTested=False, files=entries)
    (dest / f'{label}_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return dest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cli', default='arduino-cli')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--board', choices=sorted(BOARDS), default='175')
    args = parser.parse_args()
    if not 1 <= args.jobs <= 32:
        parser.error('--jobs must be between 1 and 32')
    source = ROOT / 'boards' / args.board / 'MotoNav'
    if args.board == '21':
        ensure_library(args.cli, 'lvgl', '8.3.10')
    version = re.search(r'^#define FW_VERSION "v([^\"]+)"', (source / 'src/core/FirmwareConfig.h').read_text(), re.MULTILINE).group(1)
    build = ROOT / '.build' / 'universal' / args.board
    output = build / 'output'
    output.mkdir(parents=True, exist_ok=True)
    prefix_map = '-ffile-prefix-map=' + str(Path.home()) + '=/builder'
    subprocess.run([args.cli, 'compile', '--jobs', str(args.jobs), '--libraries', str(ROOT / 'libraries'),
                    '--build-path', str(build / 'build'), '--fqbn', FQBN,
                    '--build-property', 'compiler.c.extra_flags=' + prefix_map,
                    '--build-property', 'compiler.cpp.extra_flags=' + prefix_map, '--output-dir', str(output), str(source)], check=True)
    print(package(output, ROOT / 'dist' / args.board / 'panel0', version, args.board))


if __name__ == '__main__':
    main()
