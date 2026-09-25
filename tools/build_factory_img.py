#!/usr/bin/env python3
"""Build/verify an ESP32-S3 N16R8 *destructive factory-install* full-flash .img.

This is NOT a safe update/OTA file. Every unwritten byte (NVS, other OTA slot,
LittleFS/coredump) is FF. Flash only after backing up a matching 16 MiB board.
Uses exact PlatformIO bootloader, Arduino boot_app0, partition table and app.
No SD-card image, auto-flashing, key editing, erasure or firmware compilation.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import sys

FLASH_SIZE = 16 * 1024 * 1024
SEGMENTS = (
    ('bootloader', 0x000000, 0x008000),
    ('partitions', 0x008000, 0x009000),
    ('boot_app0', 0x00E000, 0x010000),
    ('firmware', 0x010000, 0x650000),
)
# Original VQEAF N16R8 partition layout (not an inferred or universal ESP32 map).
EXPECTED_PARTITIONS = {
    'nvs': (0x9000, 0x5000),
    'otadata': (0xE000, 0x2000),
    'app0': (0x10000, 0x640000),
    'app1': (0x650000, 0x640000),
    'spiffs': (0xC90000, 0x360000),
    'coredump': (0xFF0000, 0x10000),
}

class ImageError(ValueError):
    pass


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def validate_board(root: Path) -> None:
    ini = (root / 'platformio.ini').read_text('utf-8')
    board = json.loads((root / 'boards/vqeaf_s3_n16r8.json').read_text('utf-8'))
    required = (r'(?m)^\s*\[env:vqeaf_os\]\s*$',
                r'(?m)^\s*board\s*=\s*vqeaf_s3_n16r8\s*$',
                r'(?m)^\s*board_build\.partitions\s*=\s*partitions/vqeaf_16mb_ota\.csv\s*$',
                r'(?m)^\s*board_upload\.flash_size\s*=\s*16MB\s*$')
    if not all(re.search(p, ini) for p in required):
        raise ImageError('Refusing: production environment/partition/16MB config mismatch')
    if board.get('build', {}).get('mcu') != 'esp32s3' or board.get('upload', {}).get('flash_size') != '16MB':
        raise ImageError('Refusing: board must be ESP32-S3 with 16MB Flash')
    # Confirm each section exists with the actual shipping CSV; no assumptions about other boards.
    csv_entries = {}
    for line in (root / 'partitions/vqeaf_16mb_ota.csv').read_text('utf-8').splitlines():
        line = line.partition('#')[0].strip()
        if not line:
            continue
        fields = [p.strip() for p in line.split(',')]
        if len(fields) < 5:
            raise ImageError('Invalid partition CSV line')
        csv_entries[fields[0]] = (int(fields[3], 0), int(fields[4], 0))
    if csv_entries != EXPECTED_PARTITIONS:
        raise ImageError('Refusing: partition CSV differs from audited N16R8 layout')


def validate_partition_binary(raw: bytes) -> None:
    if len(raw) != 3072 or raw[0:2] != b'\xaa\x50':
        raise ImageError('Missing or invalid 3072-byte ESP32 partition table')
    discovered = {}
    for offset in range(0, len(raw), 32):
        record = raw[offset:offset+32]
        if record[:2] == b'\xeb\xeb':
            break
        if record[:2] != b'\xaa\x50':
            raise ImageError('Unexpected partition table record magic')
        name = record[12:28].split(b'\0')[0].decode('ascii')
        if name in discovered:
            raise ImageError('Duplicate partition record')
        discovered[name] = struct.unpack_from('<II', record, 4)
    if discovered != EXPECTED_PARTITIONS:
        raise ImageError('Refusing: compiled partitions.bin differs from CSV layout')


def find_boot_app0(build_dir: Path, explicit: Path | None, framework: Path | None) -> Path:
    roots = []
    if explicit:
        roots.append(explicit)
    else:
        roots.append(build_dir / 'boot_app0.bin')
        if framework:
            roots.append(framework / 'tools/partitions/boot_app0.bin')
        core = Path(os.getenv('PLATFORMIO_CORE_DIR') or Path.home() / '.platformio')
        roots.append(core / 'packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin')
    for r in roots:
        if r.is_file():
            return r
    raise ImageError('Cannot find Arduino boot_app0.bin; specify --boot-app0 or --framework-dir')


def assemble(project_root: Path, build_dir: Path, boot_app0: Path | None,
             framework_dir: Path | None, output: Path,
             expected_firmware_sha256: str | None = None,
             expected_partitions_sha256: str | None = None) -> dict:
    validate_board(project_root)
    paths = {
        'bootloader': build_dir / 'bootloader.bin',
        'partitions': build_dir / 'partitions.bin',
        'boot_app0': find_boot_app0(build_dir, boot_app0, framework_dir),
        'firmware': build_dir / 'firmware.bin',
    }
    payloads = {}
    for name, start, end in SEGMENTS:
        try:
            data = paths[name].read_bytes()
        except OSError as exc:
            raise ImageError(f'Missing {name}: {paths[name]}') from exc
        if not data or len(data) > end-start:
            raise ImageError(f'Refusing: {name} must be 1..{end-start} bytes')
        payloads[name] = data
    if payloads['bootloader'][0] != 0xe9 or payloads['firmware'][0] != 0xe9:
        raise ImageError('Bootloader/app is not an ESP image (magic E9)')
    validate_partition_binary(payloads['partitions'])
    if expected_firmware_sha256 and sha256(payloads['firmware']).lower() != expected_firmware_sha256.lower():
        raise ImageError('Firmware SHA-256 differs from audited Release! Image NOT generated')
    if expected_partitions_sha256 and sha256(payloads['partitions']).lower() != expected_partitions_sha256.lower():
        raise ImageError('Partition SHA-256 differs from audited Release! Image NOT generated')
    if output.suffix.lower() != '.img':
        raise ImageError('Factory installer must end with .img')
    if output.exists() or output.with_suffix(output.suffix + '.json').exists():
        raise ImageError('Refusing to overwrite an existing installer/manifest')
    image = bytearray(b'\xff' * FLASH_SIZE)
    layout = []
    for name, start, end in SEGMENTS:
        data = payloads[name]
        image[start:start+len(data)] = data
        layout.append({'name': name, 'offset': hex(start), 'size': len(data),
                       'sha256': sha256(data), 'source': paths[name].name})
    output.parent.mkdir(parents=True, exist_ok=True)
    img_sha = sha256(image)
    manifest = {
        'format': 'VQEAF-N16R8-FACTORY-IMG-v1',
        'purpose': 'DESTRUCTIVE CLEAN FACTORY INSTALL; NOT OTA/SD/IN-OS INSTALLER',
        'chip': 'esp32s3', 'board': 'vqeaf_s3_n16r8', 'flash_size': FLASH_SIZE,
        'flash_offset': '0x0', 'fill': '0xFF', 'image': output.name,
        'image_sha256': img_sha, 'segments': layout,
        'unwritten_partitions': ['nvs', 'app1', 'spiffs', 'coredump'],
        'warning': 'FLASHING THIS ENTIRE 16MiB IMAGE ERASES USER NVS, OTA SLOT, LittleFS AND COREDUMPS. BACK UP FIRST.',
    }
    output.write_bytes(image)
    output.with_suffix(output.suffix + '.json').write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    verify(output, output.with_suffix(output.suffix + '.json'))
    return manifest


def verify(image_file: Path, manifest_file: Path) -> dict:
    manifest = json.loads(manifest_file.read_text(encoding='utf-8'))
    if manifest.get('format') != 'VQEAF-N16R8-FACTORY-IMG-v1' or manifest.get('flash_size') != FLASH_SIZE:
        raise ImageError('Unsupported or invalid manifest')
    image = image_file.read_bytes()
    if len(image) != FLASH_SIZE or sha256(image) != manifest.get('image_sha256'):
        raise ImageError('Image size/checksum mismatch')
    expected = {n: (s, e) for n, s, e in SEGMENTS}
    occupied = bytearray(FLASH_SIZE)
    for seg in manifest['segments']:
        name, start, length = seg['name'], int(seg['offset'], 16), seg['size']
        if name not in expected or not 0 < length <= expected[name][1]-expected[name][0] or start != expected[name][0]:
            raise ImageError('Unexpected segment address/length')
        if sha256(image[start:start+length]) != seg['sha256']:
            raise ImageError('Segment checksum mismatch: ' + name)
        occupied[start:start+length] = b'\x01' * length
    if {x['name'] for x in manifest['segments']} != set(expected):
        raise ImageError('Missing/duplicate components')
    for start in range(0, FLASH_SIZE, 65536):
        page = image[start:start+65536]
        mask = occupied[start:start+65536]
        if any(byte != 0xff for byte, used in zip(page, mask) if not used):
            raise ImageError('Unallocated Flash must be blank FF')
    validate_partition_binary(image[0x8000:0x8000+3072])
    return manifest


def cli() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--project-root', type=Path, default=Path.cwd())
    p.add_argument('--build-dir', type=Path, default=Path('.pio/build/vqeaf_os'))
    p.add_argument('--boot-app0', type=Path)
    p.add_argument('--framework-dir', type=Path)
    p.add_argument('--output', type=Path, default=Path('dist/VQEAF-OS_factory_ESP32S3_N16R8.img'))
    p.add_argument('--expected-firmware-sha256')
    p.add_argument('--expected-partitions-sha256')
    p.add_argument('--verify-only', action='store_true')
    args = p.parse_args()
    try:
        if args.verify_only:
            m = verify(args.output, args.output.with_suffix(args.output.suffix + '.json'))
            print('FACTORY IMAGE VERIFIED', m['image_sha256'])
            return 0
        m = assemble(args.project_root, args.build_dir, args.boot_app0, args.framework_dir,
                     args.output, args.expected_firmware_sha256, args.expected_partitions_sha256)
        print('FACTORY IMAGE CREATED', args.output, FLASH_SIZE, 'bytes; SHA256:', m['image_sha256'])
        print('WARNING: clean factory install ONLY. Use PlatformIO or separate firmware.bin for normal updates.')
    except (ImageError, OSError, ValueError, KeyError) as exc:
        print('FAIL:', exc, file=sys.stderr)
        return 2
    return 0

if __name__ == '__main__':
    sys.exit(cli())