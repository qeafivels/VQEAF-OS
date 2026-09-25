"""Safety and image-layout tests. Synthetic bootloader, NEVER hardware boot proof."""
from __future__ import annotations
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location('factory', Path(__file__).with_name('build_factory_img.py'))
f = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(f)


def part_bin() -> bytes:
    records = []
    subtypes = {'nvs': (1, 2), 'otadata': (1, 0), 'app0': (0, 16),
                'app1': (0, 17), 'spiffs': (1, 130), 'coredump': (1, 3)}
    for name, (offset, size) in f.EXPECTED_PARTITIONS.items():
        typ, sub = subtypes[name]
        records.append(struct.pack('<HBBII16sI', 0x50AA, typ, sub, offset, size,
                                   name.encode('ascii'), 0))
    return b''.join(records) + b'\xeb\xeb' + b'\xff' * (3072 - 2 - sum(map(len, records)))


class FactoryTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.build = self.root / '.pio/build/vqeaf_os'
        self.build.mkdir(parents=True)
        (self.root/'boards').mkdir()
        (self.root/'partitions').mkdir()
        (self.root/'platformio.ini').write_text(
            '[env:vqeaf_os]\nboard = vqeaf_s3_n16r8\nboard_upload.flash_size = 16MB\n'
            'board_build.partitions = partitions/vqeaf_16mb_ota.csv\n')
        (self.root/'boards/vqeaf_s3_n16r8.json').write_text(
            json.dumps({'build': {'mcu': 'esp32s3'}, 'upload': {'flash_size': '16MB'}}))
        (self.root/'partitions/vqeaf_16mb_ota.csv').write_text('\n'.join(
            f'{n}, data, nvs, {off:#x}, {size:#x},' for n, (off, size) in f.EXPECTED_PARTITIONS.items()))
        for name, content in {'bootloader': b'\xe9' + b'B' * 2047,
                              'partitions': part_bin(),
                              'boot_app0': b'O' * 8192,
                              'firmware': b'\xe9' + b'F' * 160000}.items():
            (self.build/(name + '.bin')).write_bytes(content)
        self.img = self.root/'dist/test.img'

    def create(self, fw=None, partitions=None):
        return f.assemble(self.root, self.build, None, None, self.img, fw, partitions)

    def test_assemble_full_blank_flash_and_verify(self):
        manifest = self.create()
        self.assertEqual(self.img.stat().st_size, 16*1024*1024)
        raw = self.img.read_bytes()
        self.assertEqual(raw[0], 0xe9)
        self.assertEqual(raw[0x8000:0x8002], b'\xaa\x50')
        self.assertEqual(raw[0xe000:0xe001], b'O')
        self.assertEqual(raw[0x10000], 0xe9)
        self.assertEqual(raw[0x650000:], b'\xff' * (len(raw)-0x650000))
        self.assertEqual(raw[0x9000:0xe000], b'\xff' * 0x5000)
        self.assertEqual(f.verify(self.img, self.img.with_suffix('.img.json'))['image_sha256'], manifest['image_sha256'])

    def test_firmware_mismatch_rejected_without_output(self):
        with self.assertRaisesRegex(f.ImageError, 'Firmware SHA-256 differs'):
            self.create(fw='0'*64)
        self.assertFalse(self.img.exists())

    def test_partition_mismatch_rejected_without_output(self):
        with self.assertRaisesRegex(f.ImageError, 'Partition SHA-256 differs'):
            self.create(partitions='0'*64)
        self.assertFalse(self.img.exists())

    def test_bad_board_rejected(self):
        p = self.root/'boards/vqeaf_s3_n16r8.json'
        p.write_text(json.dumps({'build': {'mcu': 'esp32'}, 'upload': {'flash_size': '16MB'}}))
        with self.assertRaisesRegex(f.ImageError, 'ESP32-S3'):
            self.create()

    def test_modified_partition_layout_rejected(self):
        p = self.root/'partitions/vqeaf_16mb_ota.csv'
        p.write_text(p.read_text().replace('0x10000', '0x20000'))
        with self.assertRaisesRegex(f.ImageError, 'partition CSV'):
            self.create()

    def test_bad_partition_bin_rejected(self):
        (self.build/'partitions.bin').write_bytes(b'\xff'*3072)
        with self.assertRaisesRegex(f.ImageError, 'partition table'):
            self.create()

    def test_bad_app_magic_rejected(self):
        (self.build/'firmware.bin').write_bytes(b'NOBOOT')
        with self.assertRaisesRegex(f.ImageError, 'magic'):
            self.create()

    def test_oversize_bootloader_rejected(self):
        (self.build/'bootloader.bin').write_bytes(b'\xe9'*0x8001)
        with self.assertRaisesRegex(f.ImageError, 'bootloader'):
            self.create()

    def test_verify_detects_data_in_user_partition(self):
        self.create()
        raw = bytearray(self.img.read_bytes())
        raw[0x9100] = 0x42
        self.img.write_bytes(raw)
        mpath = self.img.with_suffix('.img.json')
        m = json.loads(mpath.read_text())
        m['image_sha256'] = f.sha256(raw)
        mpath.write_text(json.dumps(m))
        with self.assertRaisesRegex(f.ImageError, 'blank FF'):
            f.verify(self.img, mpath)

    def test_second_build_never_overwrites(self):
        self.create()
        with self.assertRaisesRegex(f.ImageError, 'overwrite'):
            self.create()

if __name__ == '__main__':
    unittest.main()