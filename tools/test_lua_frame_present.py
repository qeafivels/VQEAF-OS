#!/usr/bin/env python3
"""Test pure C++ RGB565 frame policy and verify Lua presentation wiring.
Does not claim a physical ESP32-S3/LCD flicker measurement.
"""
from pathlib import Path
import re
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]


def check_structure():
    source = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
    assert 'TFT_eSprite luaCanvas(&tft)' in source, 'Lua must render offscreen'
    assert 'setAttribute(PSRAM_ENABLE' in source, 'Sprite must use PSRAM'
    assert 'luaCanvas.pushSprite(0,29)' in source, 'Flush only content viewport'
    assert 'luaFramePolicy.needsPresent(' in source, 'Skip unchanged frames'
    assert 'luaFramePolicy.invalidate()' in source, 'Repaint after OS Back dialog'
    assert 'draw.user' not in source, 'Do not bypass renderer'
    for pattern in (r'if\s*\(!luaVm\.update\(dt\)\s*\|\|\s*!luaVm\.render\(\)\s*\|\|\s*!luaPresentFrame\(\)\)',
                    r'\(!resume\s*&&\s*!luaVm\.render\(\)\)'):
        assert re.search(pattern, source), 'Frame must only flush after successful full render: ' + pattern
    assert 'if (from == ScreenId::LuaApp && s != ScreenId::LuaApp)' in source
    assert 'luaCanvas.deleteSprite()' in source


def main():
    with tempfile.TemporaryDirectory(prefix='qe-lua-frame-') as d:
        executable = Path(d)/'frame_test'
        subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-O2',
                        '-I'+str(ROOT/'src/lua'),
                        str(ROOT/'tools/lua_beta_host/test_lua_frame_policy.cpp'),
                        '-o',str(executable)],check=True)
        subprocess.run([str(executable)],check=True)
    check_structure()
    print('PASS frame policy + source wiring (host); hardware LCD flicker: NOT_RUN')

if __name__ == '__main__':
    main()
