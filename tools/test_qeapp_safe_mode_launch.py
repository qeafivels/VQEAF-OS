#!/usr/bin/env python3
"""Structural regression: never bypass trusted signed QEAPP or Safe Mode."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf8')
system=(root/'src/services/SystemService.cpp').read_text(encoding='utf8')
assert 'if (!appInstaller.get(appCtx.pendingPackageId, meta, &launchFailure))' in main
assert 'if (systemService.safeMode()) {' in main
assert 'launchFeedback="Safe Mode blocks Lua. Recovery: Leave Safe Mode"' in main
assert 'reason=SAFE_MODE action=Recovery_Leave_Safe_Mode' in main
assert 'else if (!storage.mounted()) {' in main
assert 'Lua display buffers unavailable in PSRAM' in main
assert 'Qeapp::equalHash(digest,signedHeader+84)' in main
assert 'blockedBySafeMode?"Recovery > Start normal mode"' in main
assert 'prefs.putBool("forceSafe", enabled);' in system
assert 'safe = prefs.getBool("forceSafe", false) || crashStreak >= 2;' in system
assert 'systemService.setSafeMode(false)' not in main
print("PASS QEAPP safe-mode launch error explicitly explains blocker without bypassing signatures or clearing Safe Mode")
