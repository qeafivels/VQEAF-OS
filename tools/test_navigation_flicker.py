#!/usr/bin/env python3
"""OS-wide D-pad / popup flicker regression contracts.

A structural gate is used because a host PC cannot synthesize actual ESP32-S3
GPIO edges or photograph an ST7789 screen. Hardware acceptance remains separate.
"""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
apps=(r/"src/apps/Apps.cpp").read_text(encoding="utf-8")
ui=(r/"src/core/SymbianUI.cpp").read_text(encoding="utf-8")
uih=(r/"src/core/SymbianUI.h").read_text(encoding="utf-8")
main=(r/"src/main.cpp").read_text(encoding="utf-8")
input_src=(r/"src/core/InputManager.cpp").read_text(encoding="utf-8")
launcher=(r/"src/apps/LauncherGrid.cpp").read_text(encoding="utf-8")
a=apps[apps.index("bool ThemesApp::apply("):apps.index("void ThemesApp::draw(")]
p=ui[ui.index("void SymbianUI::popupMenu("):ui.index("void SymbianUI::message(")]
checks={
 "Theme Apply never blanks LCD between its own state change and caller draw":
    "ctx.ui.clear();" not in a and
    "if (action == 0) apply(ctx);" in apps and
    "    apply(ctx); draw(ctx);" in apps,
 "Regular popup D-pad updates only previous/new focused rows":
    "if(samePopup && i!=previous && i!=selected)continue;" in p and
    "if (samePopup && previous==selected) return;" in p,
 "Popup scrolling repaints full overlay after offset changes":
    "popupCacheOffset==offset" in p and
    "if (!samePopup && count > visible)" in p,
 "Popup is invalidated when underlying content/footer changes":
    all(s in ui for s in (
        "void SymbianUI::clearContent() {\n  popupCacheValid = false;",
        "void SymbianUI::menuBackground() {\n  popupCacheValid = false;",
        "popupCacheValid = false;",
        "void SymbianUI::gridItem(",
        "void SymbianUI::listItem(")),
 "Popup cache is instance-owned and bounded":
    "bool popupCacheValid = false;" in uih and
    "popupCacheSelected = -1;" in uih and
    "popupCacheItems = nullptr;" in uih,
 "Launcher navigates via two dirty cells and skips same index":
    "ctx.ui.gridItem(old, launcherIcon[old], launcherTitle[old], false);" in launcher and
    "ctx.ui.gridItem(index, launcherIcon[index], launcherTitle[index], true);" in launcher,
 "System honors global key-modal routing, long-press exclusivity":
    "if (osBackConfirm.active())" in main and
    "GlobalShortcutPolicy::resolve(e, keyboard.active())" in main and
    "if (e.longPress) return;" in main,
 "Theme selection paints only prior/new row until scrollbar offset changes":
    "if (oldOffset != offset) draw(ctx);" in apps and
    "row(old, false); row(index, true);" in apps,
 "No screen-wide transitional wipe when entering heavy routes":
    "if (animate && !heavyRoute && from != ScreenId::Launcher" in main,
 "Full-screen clear stays disabled for Lua compositor":
    "&& s != ScreenId::LuaApp" in main,
}
for name,ok in checks.items():
    assert ok,name
    print("PASS",name)
print("LIMIT: structural/host gates cannot establish physical D-pad contact bounce or actual LCD ghosting")
