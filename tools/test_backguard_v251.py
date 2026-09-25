#!/usr/bin/env python3
"""Core-only Back patch: state machine host test and strict unmodified UI evidence."""
from pathlib import Path
import hashlib, subprocess, sys, json
r=Path(__file__).resolve().parents[1]
out=r/'build_reports/backguard';out.mkdir(parents=True, exist_ok=True)
src=r/'src/main.cpp'
s=src.read_text(encoding='utf8')
required=[
 '#include "core/OsBackConfirm.h"',
 'if (osBackConfirm.active()) {',
 'if (osBackConfirm.begin(screen, next, e)) {',
 'pixelSnakeApp.tick(appCtx)',
 'stopwatchApp.tick(appCtx, screen == ScreenId::Stopwatch && !osBackConfirm.active());',
 '&& !osBackConfirm.active()',
 'enterScreen(screen, false, true);',
 'ui.dialog("VQEAF OS", "Close application?", "", "Yes", "No", osBackConfirm.selected());',
 'if (s != from && osBackConfirm.active()) osBackConfirm.cancel();',
]
for item in required:
 assert item in s, f'Integration missing: {item}'
exe=out/'test_os_back_confirm'
cmd=['g++','-std=c++11','-Wall','-Wextra','-Werror','-Itools/host_stubs',
     'tools/backguard_host/test_os_back_confirm.cpp','-o',str(exe)]
p=subprocess.run(cmd,cwd=r,check=True,capture_output=True,text=True)
p=subprocess.run([str(exe)],cwd=r,check=True,capture_output=True,text=True)
print(p.stdout.strip())
print('Static input/router/renderer integration PASS')
