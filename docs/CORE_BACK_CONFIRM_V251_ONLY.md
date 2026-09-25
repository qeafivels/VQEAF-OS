# VQEAF OS 2.5.1 — Core-only Back confirmation patch

This is a **behavior-only** update based on `VQEAF_OS_v2.5.1_QEAPP_Launch_Icons_Memory_FPS_Full_Source.zip`.

**Visual contract: unchanged.** The OS keeps its exact existing v2.5.1 Retro-Go-style launcher, icon atlas, color palette, font, graphical layout, splash, sprite renderer, themes and transition code. This patch does **not** copy the Nokia 2700 UI or the video artwork. It reuses the **existing** `SymbianUI::dialog()` to show a confirmation without adding a framebuffer.

## What changes

- Pressing physical Back `[A]` or the app-mapped right-softkey `[B]` when an app **actually requests exit** now opens the existing `VQEAF OS` confirmation dialog: `Close application?` — `Yes / No`.
- `No` is selected by default to avoid accidental exit. `Left`/`Up` selects Yes; `Right`/`Down` selects No. `OK` confirms. `A`, `B` or `OPTION` cancels and redraws the current app **without calling its enter/reset routine**.
- The app-local Back handler runs **first**. Leaving Gallery preview, closing Music now-playing, dismissing application options, editing a text field, etc. stays as it was. App Installer/Theme Manager/Recovery/Launcher are not changed and retain their own confirmation/navigation behavior.
- Confirmed exits continue to the **actual** original destination chosen by the app (Applications, Launcher, Collection, Files or Idle). No guessed back destination.
- While modal: game frame updates, Stopwatch repaint, Music UI progress and Gallery slideshow drawing are suppressed (audio stream continues), and system WiFi/clock badge updates are deferred so the popup is not overwritten. On cancel the current app redraws in `resume` mode.
- Card removal, automatic lock and any other forced screen transition invalidate a pending modal so it cannot later exit the wrong screen.
- No new keypad GPIOs, display assets, themes, icons, package formats, install/signature code, sound decoder, or rendering primitives.

### In-scope app exits

`Snake`, `Browser`, `Music`, `Gallery`, `TextViewer`, `Shell`, `Notes`, `Calculator`, `Stopwatch`. Other screens retain existing Back semantics, including `Launcher`/`Explorer`, `Idle`, `Settings`, `Themes`, `AppInstaller`, and `Recovery`.

### Build and reproduce

```bat
py -3 tools/test_backguard_v251.py
py -3 tools/verify_v251.py
pio run -e vqeaf_os
pio run -e vqeaf_os -t upload
pio device monitor --baud 115200
```

`pio` and the ESP32-S3 board/toolchain are required for a real-device firmware build; PC mock tests alone are not a board verification. The existing serial `[VQEAF][FPS]` and `[VQEAF][PERF]` optional diagnostics still work.

### Device acceptance

1. Launch signed Snake. Press Back; verify popup has No preselected, selecting No returns to same game progress, and there is no accidental restart.
2. Press Back again, choose Yes and OK; verify Applications appears and that Snake's saved high score persists.
3. Launch Qeafbrowser from the old launcher: pressing Back asks only when actually leaving the app, not while a popup or text input is active. Cancel preserves the loaded URL/scroll.
4. Enter Gallery preview and Music now-playing and press Back: first level of Back remains local; only the subsequent **real exit** opens the system popup.
5. During popup, audio playback continues, gallery slide and game frames are not repainted on top of popup. Close the popup and verify they resume.
6. While popup is displayed, remove test microSD to force fallback, or wait for automatic lock (if configured); confirm the old popup cannot later exit the replacement screen.
7. Check optional 115200 logs: `[VQEAF][BACK] requested`, `dialog selected`, `accepted` or `cancelled`. Check RGB565 and FPS on real LCD separately; no board data is claimed here.

**Limitations:** Real ESP32-S3 LCD, I2S audio, actual frame/energy measurements and physical keypad behavior remain to be checked with connected hardware. Host compile and pixel/installer regressions do not substitute for these checks.
