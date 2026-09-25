#!/usr/bin/env python3
"""Dependency-free smoke for the PlatformIO FS pre-script; NOT a firmware build."""
from pathlib import Path
from tempfile import TemporaryDirectory

script = Path(__file__).with_name("pio_fs_sdmmc_dependency.py").read_text(encoding="utf-8")

class FakePlatform:
    def __init__(self, root):
        self.root = root
    def get_package_dir(self, name):
        assert name == "framework-arduinoespressif32"
        return str(self.root)

class FakeEnv:
    def __init__(self, root):
        self.platform = FakePlatform(root)
        self.include_paths = []
    def PioPlatform(self):
        return self.platform
    def AppendUnique(self, **kw):
        self.include_paths.extend(kw.get("CPPPATH", ()))

with TemporaryDirectory() as d:
    root = Path(d)
    (root / "libraries/FS/src").mkdir(parents=True)
    (root / "libraries/FS/src/FS.h").write_text("// mock", encoding="utf-8")
    fake = FakeEnv(root)
    exec(compile(script, "pio_fs_sdmmc_dependency.py", "exec"),
         {"Import": lambda name: None, "env": fake})
    assert fake.include_paths == [str(root / "libraries/FS/src")]

    (root / "libraries/FS/src/FS.h").unlink()
    failed = False
    try:
        exec(compile(script, "pio_fs_sdmmc_dependency.py", "exec"),
             {"Import": lambda name: None, "env": FakeEnv(root)})
    except RuntimeError as e:
        failed = "FS.h" in str(e)
    assert failed, "missing framework FS.h must stop the build"
print("PASS: PlatformIO Arduino FS include path and missing-header guard")
