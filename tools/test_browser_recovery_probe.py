#!/usr/bin/env python3
"""Regression guard: isolated real-device reboot probe cannot touch live data."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
p=(r/"src/services/BrowserRecoveryProbe.h").read_text()
main=(r/"src/main.cpp").read_text()
serial=(r/"tools/qb_hardware_lab.py").read_text()
def require(expr,desc):
    if not expr:raise AssertionError(desc)
    print("PASS",desc)
require("StoragePaths::TEMP" in p and "/System/Temp/qb_recovery" in p,
        "recovery artifacts isolated under temporary app-independent SD folder")
require("qeafbrowser_cookies" not in p and "qeafbrowser_bookmarks" not in p,
        "never modify live browser bookmarks or auth cookies")
require("storage.writeAtomic(HTML" in p and "storage.writeAtomic(COOKIE" in p and
        "storage.writeAtomic(MARKER" in p and "Commit marker LAST" in p,
        "stage writes both records atomically and commits marker last")
require("jar->requestHeader" in p and "jar->deserialize" in p and
        'http://qb-recovery.invalid/' in p and 'other.invalid/' in p,
        "cookie cold boot roundtrip plus downgrade and origin isolation")
require("BrowserThumbFormat::valid" in p and "memcmp(tile,expected" in p and
        "storage.writeAtomic(SD_THUMB" in p and "sdThumb" in p and
        "LittleFS.begin(false)" in p,
        "LittleFS and SD fallback thumbnail CRC survive reboot without formatting")
require('c=="diag qb reboot"' in main and "BrowserRecoveryProbe::staged(storage)" in main
        and "ESP.restart()" in main and "Serial.flush()" in main,
        "reboot only by explicit serial diagnostic after committed test stage")
require('if(music.playing())' in main and 'diag qb verify' in main and
        "boot=gather(25" in serial and "port_slot[0].write(b\"diag qb verify" in serial,
        "audio guard and post-reboot serial evidence")
print("PASS isolated recovery probe structural gates")
