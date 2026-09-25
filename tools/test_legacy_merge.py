#!/usr/bin/env python3
"""Compile native E524546 feature integration, assert OS routing & theme catalog."""
from pathlib import Path
import subprocess,sys,tempfile,re
R=Path(__file__).resolve().parents[1]
def read(path):return (R/path).read_text(encoding="utf8")
with tempfile.TemporaryDirectory(prefix="vqeaf-legacy-") as temp:
    exe=Path(temp)/"sketchpad"
    subprocess.run(["g++","-std=c++11","-O1","-Wall","-Wextra","-Werror",
                    "-Isrc","tools/legacy_host/test_sketchpad.cpp",
                    "-o",str(exe)],cwd=R,check=True)
    subprocess.run([str(exe)],check=True,timeout=20)
m=read("src/main.cpp"); apps=read("src/apps/Apps.cpp")
types=read("src/core/Types.h");sys_service=read("src/services/SystemService.cpp")
storage=read("src/services/StorageService.cpp"); app=read("src/apps/SketchpadApp.cpp")
assert m.count("case ScreenId::Sketchpad:")==2
assert 'static SketchpadApp sketchpadApp;' in m
assert '#include "apps/SketchpadApp.h"' in m
assert "  Sketchpad //" in types
assert re.search(r'APP_COUNT\s*=\s*15',apps)
assert '"Sketchpad","Notifications"' in apps
assert 'if(index==12)ctx.pendingFolderPath' in apps
assert 'ScreenId::Sketchpad,nullptr' in apps
assert 'case ScreenId::Sketchpad:' in sys_service
assert "StoragePaths::SKETCHPAD" in storage
assert "writeAtomic(filePath()" in app
assert "recoverAtomicFile(dest)" in app
assert "heap_caps_free(buffer)" in app
assert "model_.importLegacy(buffer,size)" in app
assert "if(dirty_)" in app
assert "ctx.ui.popupMenu(choices,3" in app
a=read("themes/notebook_legacy.vqeaf")
b=read("sd/System/Themes/notebook_legacy.vqeaf")
assert a==b and "@vqeaf 1.0" in a and "palette {" in a and "launcher {" in a
assert len(a.encode())<8192
print("PASS: native Sketchpad C++ model + main routing + sd transaction policy + Notebook theme catalog")
