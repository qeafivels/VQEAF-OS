#!/usr/bin/env python3
"""OS-wide keyboard/USB/SD structural regression plus compiled USB debounce tests."""
from pathlib import Path
import subprocess
import tempfile
r=Path(__file__).resolve().parents[1]
keyboard=(r/"src/core/TextKeyboard.cpp").read_text()
header=(r/"src/core/TextKeyboard.h").read_text()
main=(r/"src/main.cpp").read_text()
board=(r/"include/BoardConfig.h").read_text()
browser=(r/"src/apps/Apps.cpp").read_text()
assert '"qwertyuiop"' in keyboard and '"asdfghjkl"' in keyboard and '"zxcvbnm./-"' in keyboard
assert all(s in keyboard for s in ('"SHIFT"','"SYM"','"SPACE"','"DEL"','"DONE"','".com"','.net"','.org"'))
assert "ui.clearContent()" not in keyboard, "keyboard must not expose blank LCD frame"
assert "MAX_TEXT=159" in header and "text.length()+n>MAX_TEXT" in keyboard
assert 'ctx.keyboard.open("Web address"' in browser and "ctx.keyboard.draw(ctx.ui" in browser
assert 'notifications.push("USB connected"' in main
assert 'notifications.push("USB disconnected"' in main
assert 'notifications.push("microSD removed"' in main
assert 'notifications.push("microSD mounted"' in main
assert 'notifications.push("microSD detected"' in main
assert "HWCDC::isPlugged()" in main and "VQEAF_HAS_NATIVE_USB_HOST_DETECTION" in main
assert "TFT_LEDK_PIN = 39" in board and "SD_D0  = 9" in board
code=r"""
#include "services/UsbLinkMonitor.h"
#include <cassert>
#include <cstdio>
int main(){
  UsbLinkMonitor m;
  using E=UsbLinkMonitor::Event;
  assert(m.sample(false,0)==E::None);
  assert(m.sample(true,100)==E::None);
  assert(m.sample(false,200)==E::None); // electrical bounce suppressed
  assert(m.sample(true,300)==E::None);
  assert(m.sample(true,1049)==E::None);
  assert(m.sample(true,1050)==E::HostConnected);
  assert(m.hostConnected());
  assert(m.sample(true,2000)==E::None);
  assert(m.sample(false,2100)==E::None);
  assert(m.sample(true,2300)==E::None);
  assert(m.sample(false,2400)==E::None);
  assert(m.sample(false,3150)==E::HostDisconnected);
  assert(!m.hostConnected());
  // Wrap-safe relative millisecond arithmetic.
  UsbLinkMonitor w;
  assert(w.sample(false,0xfffffe00U)==E::None);
  assert(w.sample(true,0xffffff00U)==E::None);
  assert(w.sample(true,0x000001f0U)==E::HostConnected);
  puts("PASS: USB attach/detach debounced + wrap");
}
"""
with tempfile.TemporaryDirectory(prefix="vqeaf-usb-") as tmp:
    p=Path(tmp)
    (p/"test.cpp").write_text(code)
    subprocess.run(["g++","-std=c++11","-Wall","-Wextra","-Werror","-Isrc",
                    str(p/"test.cpp"),"-o",str(p/"test")],check=True,cwd=r)
    subprocess.run([str(p/"test")],check=True)
print("PASS: Qeafbrowser-style keyboard + USB/SD OS notices structural gates")
