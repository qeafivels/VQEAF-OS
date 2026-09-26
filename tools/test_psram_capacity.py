#!/usr/bin/env python3
"""Host gate: N16R8 heap-reported size tolerates reserved allocator metadata."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
code=r"""
#include "core/PsramCapacity.h"
#include <cassert>
int main() {
 using VqeafMemory::hasN16R8Psram;
 assert(hasN16R8Psram(true, 8386279U));  // real COM3 boot sample
 assert(hasN16R8Psram(true, 8388608U));  // ideal full 8 MiB
 assert(hasN16R8Psram(true, 8388608U - 65536U)); // margin edge
 assert(!hasN16R8Psram(true, 8388608U - 65537U));
 assert(!hasN16R8Psram(true, 4194304U)); // 4 MiB
 assert(!hasN16R8Psram(false, 8388608U)); // chip detection required
 assert(!hasN16R8Psram(false, 0));
}
"""
with tempfile.TemporaryDirectory() as temp:
 source=Path(temp)/"gate.cpp";binary=Path(temp)/"gate.exe"
 source.write_text(code,encoding="utf-8")
 subprocess.run(["g++","-std=c++11","-Wall","-Wextra","-Werror",
                 "-I",str(root/"src"),str(source),"-o",str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
print("PASS: N16R8 PSRAM real reported size, missing PSRAM and threshold boundaries")
