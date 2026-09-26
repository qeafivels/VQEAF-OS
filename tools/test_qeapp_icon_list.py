#!/usr/bin/env python3
"""Regression contract for verified QEAPP/2 app icons; offline, read-only.

Tests that installed icons still require their signed receipt SHA-256 and
that Inbox thumbnail rendering is progressive, bounded, PSRAM-backed and
cryptographically verified rather than trusting an unverified SD bitmap.
"""
from pathlib import Path
import argparse
import hashlib
import struct

ROOT=Path(__file__).resolve().parents[1]
app=(ROOT/"src/apps/Apps.cpp").read_text(encoding="utf-8")
main=(ROOT/"src/main.cpp").read_text(encoding="utf-8")
hdr=(ROOT/"src/apps/Apps.h").read_text(encoding="utf-8")
installer=(ROOT/"src/services/AppInstallerService.cpp").read_text(encoding="utf-8")
blit=(ROOT/"src/core/QeappIconBlit.h").read_text(encoding="utf-8")
geometry=(ROOT/"src/core/UiLayoutGeometry.h").read_text(encoding="utf-8")

checks={
 "App Inbox processes at most one package per tick":
   'void AppInstallerApp::tick(' in app and
   'lastIconCheckAt=now;' in app and
   'inspectWithIcon(' in app[app.index('void AppInstallerApp::tick('):app.index('void AppInstallerApp::draw(')],
 "Signed verified manifest and 32x32 icon are cached together":
   'if(verifiedPackage && iconReady)' in app and
   'label.manifestOk=true;' in app and
   'iconCacheState[row]=1;' in app,
 "Cannot display unverified Inbox pixels":
   'const bool custom=inboxIcons && iconCacheState[row]==1;' in app,
 "Inbox cache is invalidated on every reload":
   'iconCacheIndex[slot]=-1;iconCacheState[slot]=0;' in app,
 "PSRAM icon allocations do not use Arduino loop stack":
   'MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT' in app[app.index('void AppInstallerApp::enter('):app.index('void AppInstallerApp::paintRow(')],
 "Installed listing still reads verified icon":
   app.count('ctx.installer.loadIcon(')>=4,
 "Installed icon SHA-256 is compared to verified receipt":
   'return Qeapp::equalHash(digest,trusted->verifiedIconHash);' in installer,
 "Signature verification is required before Inbox icon preview":
   'const bool ok=scanPackage(f,h,meta,error,true);' in installer,
 "FIFO token per visible row prevents stale icon on scroll":
   'if(iconCacheIndex[row]!=item)' in app,
 "Background check executes only while installer is foreground":
   'appInstallerApp.tick(appCtx,screen==ScreenId::AppInstaller && !osBackConfirm.active());' in main,
 "COM3 read-only installed icon diagnostic available":
   'diag app icons' in main and 'signed_icon=%u verified_pixels=%u' in main,
 "LCD blit preserves SPI endian mode":
   'const bool previous = lcd.getSwapBytes();' in blit and
   'lcd.setSwapBytes(previous);' in blit,
 "Row bounds fit 32x32 preview":
   'LIST_ROW_H=42' in geometry and 'LIST_VISIBLE=6' in geometry,
}
for description,ok in checks.items():
    assert ok, description
    print("PASS",description)

parser=argparse.ArgumentParser()
parser.add_argument("packages",nargs="*",type=Path,
  help="Optional local .qeapp files for 32x32 icon section digest checks")
args=parser.parse_args()
for file in args.packages:
    b=file.read_bytes()
    assert b.startswith(b'QEAPP2\r\n')
    m,i,p=struct.unpack_from("<III",b,8)
    assert i==2048 and len(b)==116+m+i+p+76
    icon=b[116+m:116+m+i]
    assert hashlib.sha256(icon).digest()==b[52:84]
    assert len(set(icon))>1, "empty/trivial icon"
    print("PASS 32x32 RGB565 LE signed package section",file.name)
