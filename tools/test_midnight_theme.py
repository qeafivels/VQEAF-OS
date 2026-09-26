#!/usr/bin/env python3
"""Theme/typography regression for the 240x320 Midnight default.
No hardware or private themes required.
"""
from pathlib import Path
import re
r=Path(__file__).resolve().parents[1]
theme=(r/"src/core/Theme.h").read_text(encoding="utf-8")
settings=(r/"src/services/SettingsStore.cpp").read_text(encoding="utf-8")
defaults=(r/"src/services/SettingsStore.h").read_text(encoding="utf-8")
ui=(r/"src/core/SymbianUI.cpp").read_text(encoding="utf-8")
apps=(r/"src/apps/Apps.cpp").read_text(encoding="utf-8")
apph=(r/"src/apps/Apps.h").read_text(encoding="utf-8")
glyphs=(r/"src/core/UiVietnameseFont.h").read_text(encoding="utf-8")
checks={
 "Stable numeric NVS identifiers; Midnight appended":
    all(x in theme for x in ("Classic = 0","Black = 1","S60Green = 2",
        "AmoledRed = 3","External = 4","ModernDark = 5")),
 "Fresh installation defaults to Midnight":
    "ThemeId theme = ThemeId::ModernDark;" in defaults and
    'prefs.isKey("theme")' in settings,
 "Migration is versioned and does not erase external theme paths":
    'prefs.putUChar("themeRev",4)' in settings and
    'themeRev<4&&saved!=static_cast<uint8_t>(ThemeId::External)' in settings and
    'prefs.remove("themeFile")' not in settings[:settings.index('void SettingsStore::save()')],
 "All five themes selectable; reset defaults to Midnight":
    "static constexpr int BUILTIN_COUNT = 5;" in apph and
    "static const ThemeId builtinThemeIds[5]" in apps and
    "s.theme = ThemeId::ModernDark;" in apps and
    'ThemeId::ModernDark,ThemeId::Classic' in apps,
 "Midnight ASCII AND UTF-8 share one DejaVu-based raster":
    ui.count('themeId==ThemeId::ModernDark || UiVietnameseFont::hasUtf8')>=4 and
    "UiVietnameseFont::measure(text.c_str()" in ui,
 "Documented 11/13px regular/bold baseline avoids list clipping":
    "11,14" in (r/"tools/generate_vietnamese_glyphs.py").read_text(encoding="utf-8") and
    "LIST_DETAIL_OFFSET_Y=22;" in (r/"src/core/UiTypography.h").read_text(encoding="utf-8") and
    "REGULAR_GLYPH_H=18;" in (r/"src/core/UiTypography.h").read_text(encoding="utf-8"),
 "Pixel-safe Midnight ellipsis and popup focus use palette tokens":
    'const String marker=themeId==ThemeId::ModernDark ? "..." : "~";' in ui and
    "themeId==ThemeId::ModernDark?colors.accent:TFT_WHITE" in ui and
    "themeId==ThemeId::ModernDark?colors.border:0xBDF7" in ui,
 "Prominent 2x modern clock uses the same safe DejaVu bitmap":
    "inline int drawScaled(" in glyphs and ui.count("UiVietnameseFont::drawScaled(")>=3,
 "Chronometer, main menu, footer and lists use modern font metrics":
    "textWidth(center,UiTypography::BODY)" in ui and
    "textWidth(topTime,UiTypography::MICRO)" in ui and
    "textWidth(label, UiTypography::CAPTION)" in ui and
    "fitTextPixels(ttl, UiTypography::BODY" in ui,
 "No initial Classic splash and no manual theme resets needed":
    "ThemeId themeId = ThemeId::ModernDark;" in (r/"src/core/SymbianUI.h").read_text(encoding="utf-8"),
 "COM3 read-only theme mode acceptance":
    'diag theme status' in (r/"src/main.cpp").read_text(encoding="utf-8") and
    'modern_font=%u' in (r/"src/main.cpp").read_text(encoding="utf-8"),
 "Midnight has no wallpaper framebuffer":
    "vector-only atmospheric bands" in ui and
    "static const uint16_t shades[]" in ui,
 "Safe Mode/legacy themes preserved":
    "themeId = ThemeId::External;" in ui and
    'return "VQEAF Lime"' in theme,
}
for name,ok in checks.items():
    if not ok:raise AssertionError(name)
    print("PASS",name)

# Extract exact built-in Midnight RGB565 palette and calculate contrast in RGB888.
start=theme.index("if (id == ThemeId::ModernDark)")
end=theme.index("};",start)
palette=[int(x,16) for x in re.findall(r"0x[0-9A-Fa-f]{4}",theme[start:end])]
assert len(palette)==13, f"Expected 13 theme slots, got {len(palette)}"
def rgb565(x):
    return ((x>>11&31)*255/31,(x>>5&63)*255/63,(x&31)*255/31)
def luminance(x):
    channels=[]
    for c in rgb565(x):
        c=c/255
        channels.append(c/12.92 if c<=0.04045 else ((c+0.055)/1.055)**2.4)
    return sum(x*y for x,y in zip(channels,(0.2126,0.7152,0.0722)))
def contrast(a,b):
    a,b=sorted((luminance(a),luminance(b)),reverse=True)
    return (a+0.05)/(b+0.05)
for label,fg,bg in (
    ("body",palette[3],palette[0]),("header",palette[6],palette[5]),
    ("secondary",palette[4],palette[0]),("selected",palette[3],palette[2]),
    ("popup",palette[10],palette[9]),
    ("secondary-on-card",palette[4],palette[1]),
    ("focus-accent-on-selection",palette[7],palette[2])):
    ratio=contrast(fg,bg)
    assert ratio>=4.5,(label,ratio)
    print(f"PASS {label} LCD palette contrast: {ratio:.2f}:1")
# Sanity-check both pre-baked bitmap roles include common Vietnamese NFC.
for cp in ("0x0041","0x0061","0x00E1","0x1EA1","0x1EC7"):
    assert glyphs.count("{"+cp+",")>=2, cp
print("PASS regular/bold glyph sets cover basic Latin + Vietnamese NFC")

# Font atlas completeness: both roles have precisely the same supported points.
rg=re.compile(r"^\{0x([0-9A-F]{4}),\s*(\d+),\s*\{([^}]+)\}\},",re.M)
found=rg.findall(glyphs)
assert len(found)>=450,len(found)
half=len(found)//2
regular=found[:half]; bold=found[half:]
assert len(regular)==len(bold)==229 and [x[0] for x in regular]==[x[0] for x in bold]
assert all(1<=int(x[1])<=16 and len(x[2].split(','))==18 for x in found)
assert "role==1?18:15" not in glyphs and "tft.fillRect(x,y,min(clipW,count),18,bg)" in glyphs
print("PASS exact 229 Latin/NFC Vietnamese regular + bold glyphs, 18-row safe draw")
