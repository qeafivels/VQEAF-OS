#!/usr/bin/env python3
"""Static integration regression for Qeafbrowser native host port.

These assertions check routing and security boundaries, not real HTTP/TLS or TFT timings.
The CI workflow also compiles the entire stock OS and Lua beta for target ESP32-S3.
"""
from pathlib import Path
import re

root=Path(__file__).resolve().parents[1]
service=(root/"src/services/BrowserService.cpp").read_text()
api=(root/"src/services/BrowserService.h").read_text()
app=(root/"src/apps/Apps.cpp").read_text()
main=(root/"src/main.cpp").read_text()
pio=(root/"platformio.ini").read_text()

def gate(ok, why):
    if not ok: raise AssertionError(why)

for route in ("mtt:start","mtt:history","mtt:bookmark","mtt:help","mtt:about"):
    gate(route in service, "route missing: "+route)
gate("validWebUrl(url.c_str())" in service, "bookmark loading must validate untrusted SD data")
gate("storage->recoverAtomicFile(kBrowserBookmarksPath)" in service, "recover interrupted bookmark writes")
gate("storage->writeAtomic(kBrowserBookmarksPath" in service, "bookmark persistence must be atomic")
gate("storage->mounted()" in service, "storage must be optional")
gate("BOOKMARK_MAX = 12" in api and "HISTORY_MAX = 16" in api, "bounded pools changed")
gate("MALLOC_CAP_SPIRAM" in service, "browser pool must use PSRAM")
gate("if (!strncmp(targetUrl,\"mtt:\",4)) return renderInternal(targetUrl, addHistory);" in service,
     "mtt must load even without WiFi")
gate("!strcmp(name,\"anchor\")" in service and "!strcmp(name,\"go\")" in service,
     "WML anchor / nested go support missing")
gate("TrustedTls::configure(secure" in service, "do not regress TLS verification")
gate("Unsafe HTTPS downgrade blocked" in service, "HTTPS redirect downgrade protection missing")
gate("MAX_DOWNLOAD = 4 * 1024 * 1024" in service, "download cap missing")
gate("return ScreenId::AppInstaller" in app and "return ScreenId::Themes" in app,
     "signed app and theme files must still go through existing managers")
gate("ctx.browser.bookmarkCurrent()" in app and "ctx.browser.load(dest)" in app,
     "OS native browser menu not wired")
gate("if(ctx.browser.lineCount()==0) loadHome(ctx);" in app, "Speed Dial should work offline")
gate("static BrowserService browserService;" in main, "OS should retain one shared browser service")
gate("vqeaf_os" in pio, "OS build profile must remain")
gate("vqeaf_lua_beta" in pio, "existing Lua beta profile must remain")
print("PASS Qeafbrowser native integration structural gates")
