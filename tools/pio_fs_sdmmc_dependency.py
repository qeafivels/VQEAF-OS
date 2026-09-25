"""PlatformIO pre-script: expose the framework's FS headers to SD_MMC.
LDF deep+ can discover both libraries but omit SD_MMC -> FS as a dependency,
causing SD_MMC.cpp to fail with "FS.h: No such file or directory".
This adds only an include search path; it does not vendor or patch Arduino.
"""
from pathlib import Path

Import("env")

framework = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
if not framework:
    raise RuntimeError("Arduino ESP32 framework package not installed")
fs_include = Path(framework) / "libraries" / "FS" / "src"
if not (fs_include / "FS.h").is_file():
    raise RuntimeError("Arduino ESP32 FS.h missing at " + str(fs_include))
env.AppendUnique(CPPPATH=[str(fs_include)])
print("[VQEAF][PIO] Arduino FS include: " + str(fs_include))
