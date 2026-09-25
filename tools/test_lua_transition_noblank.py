#!/usr/bin/env python3
"""Regression guard: no deliberate black/interstitial frame around Lua rendering.

Structural only: this cannot measure ST7789 tear lines, power or backlight PWM.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
src = (root / "src/main.cpp").read_text(encoding="utf-8")
policy = (root / "src/lua/QeLuaFramePolicy.h").read_text(encoding="utf-8")

def check(ok, reason):
    if not ok:
        raise AssertionError(reason)

check("s != ScreenId::LuaApp" in src[src.index("// Avoid a physical blank frame"):src.index("switch (s)", src.index("// Avoid a physical blank frame"))],
      "enterScreen must not clear LCD immediately before Lua first push")
route = src[src.index("#if defined(VQEAF_V250_NAV_COMPAT)"):src.index("if (animate && !heavyRoute")]
check("from==ScreenId::LuaApp || s==ScreenId::LuaApp" in route,
      "Lua entry and exit must suppress transitionOut and Opening interstitial")
check("ui.transitionOut()" in src and "if (animate && !heavyRoute" in src,
      "existing transition guard must still protect regular UI")
check("luaCanvas.pushSprite(0,29)" in src and "luaFramePolicy.needsPresent(" in src,
      "completed Lua frames must still be buffered and gated")
check("if (resume) luaFramePolicy.invalidate()" in src,
      "Back cancellation must force restoring covered Lua viewport")
check("memcmp(drawing, previous, kBytes)" in policy,
      "unchanged sprite must not be retransmitted")
check(re.search(r"if\s*\(!luaVm\.update\(dt\)\s*\|\|\s*!luaVm\.render\(\)\s*\|\|\s*!luaPresentFrame\(\)\)", src) is not None,
      "flush must follow successful update and render")
print("PASS: Lua transitions do not intentionally expose blank content (static host gate)")
