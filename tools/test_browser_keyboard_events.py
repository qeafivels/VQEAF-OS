#!/usr/bin/env python3
"""Execute REAL TextKeyboard.cpp + NotificationService.cpp under Arduino host mocks.
Browser dispatch remains a source-contract check; LCD/USB/SD still need hardware.
"""
import subprocess,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
browser=(root/"src/apps/Apps.cpp").read_text()
main=(root/"src/main.cpp").read_text()
storage=(root/"src/services/StorageService.cpp").read_text()
assert 'ctx.keyboard.open("Web address"' in browser
assert 'if(ctx.keyboard.active()){if(ctx.keyboard.handle(e))' in browser
assert 'ctx.browser.load(u)' in browser
assert 'if(ctx.keyboard.accepted())' in browser
assert 'notifications.push("USB connected"' in main
assert 'notifications.push("USB disconnected"' in main
assert 'notifications.push("microSD removed"' in main
assert 'notifications.push("microSD mounted"' in main
assert 'return CardEvent::Removed;' in storage
assert 'return CardEvent::Mounted;' in storage
code=r"""
#include "core/TextKeyboard.h"
#include "core/SymbianUI.h"
#include "services/NotificationService.h"
#include "services/UsbLinkMonitor.h"
#include <cassert>
#include <cstdio>
#include <string>
uint32_t vqeafFakeMillis=0;
// Link-only host stubs: keyboard drawing is not invoked by this key-event test.
void SymbianUI::chrome(const String &,bool,bool,bool,bool){}
void SymbianUI::softkeys(const String &,const String &,const String &){}
static KeyEvent k(Key key){return KeyEvent(key,true,false,false);}
static void send(TextKeyboard &kb,Key key){assert(kb.handle(k(key)));}
static void up(TextKeyboard &kb,int n){while(n--)send(kb,Key::Up);}
int main(){
  TextKeyboard kb;
  kb.open("Web address");
  // QWERTY starts on q. Typing, D-pad focus and Backspace.
  send(kb,Key::Start);
  send(kb,Key::Right);send(kb,Key::Start);
  assert(kb.value()=="qw");
  send(kb,Key::B); assert(kb.value()=="q");
  // Focus wraps within row.
  for(int i=0;i<9;++i)send(kb,Key::Right);
  send(kb,Key::Start);assert(kb.value()=="qq");
  // Switch case with OPTION: last key currently q.
  send(kb,Key::Option);send(kb,Key::Start);assert(kb.value()=="qqQ");
  // Action-row suffix shortcut and result are available outside passwords.
  kb.open("Web address","https://qeafivels");
  up(kb,2); // q row -> TLD row via wrap
  send(kb,Key::Start);assert(kb.value()=="https://qeafivels.com");
  send(kb,Key::Menu);assert(!kb.active()&&kb.accepted());
  kb.open("WiFi password","secret",true);
  // masked keyboard skips URL shortcuts (row 5).
  up(kb,2); // q row -> special action row via wrap
  send(kb,Key::Start); // SHIFT
  send(kb,Key::Down); // special -> number row
  send(kb,Key::Down); // number -> q
  send(kb,Key::Start);assert(kb.value()=="secretQ");
  send(kb,Key::A);assert(!kb.active()&&kb.cancelled()&&!kb.accepted());
  kb.open("Limit",String(std::string(159,'x')));
  assert(kb.value().length()==159);
  send(kb,Key::Start);assert(kb.value().length()==159);
  // Multi-tap is still accepted in the universal keyboard.
  kb.open("T9");
  send(kb,Key::Num2);assert(kb.value()=="a");
  vqeafFakeMillis=100;send(kb,Key::Num2);assert(kb.value()=="b");
  vqeafFakeMillis=1000;send(kb,Key::Num2);assert(kb.value()=="ba");
  NotificationService notes;
  notes.push("microSD detected","Memory card ready");
  notes.push("microSD removed","Storage offline");
  notes.push("microSD mounted","Filesystem ready");
  notes.push("USB connected","Computer connected via Type-C");
  notes.push("USB disconnected","Type-C data connection lost");
  assert(notes.count()==5&&notes.unreadCount()==5);
  assert(notes.at(0).title=="USB disconnected");
  assert(notes.at(1).title=="USB connected");
  assert(notes.at(2).title=="microSD mounted");
  notes.markRead(0);assert(notes.unreadCount()==4);
  puts("PASS: Browser OS TextKeyboard keypresses, TLD, password, cancel, T9, notifications");
}
"""
with tempfile.TemporaryDirectory(prefix="vqeaf-keyboard-") as td:
    src=Path(td)/"test.cpp"
    src.write_text(code)
    exe=Path(td)/"test"
    subprocess.run(["g++","-std=c++11","-O0","-Wall","-Wextra","-Werror",
                    "-DVQEAF_INPUT_FAKE_CLOCK",
                    "-ffunction-sections","-fdata-sections",
                    "-Itools/host_stubs","-Iinclude","-Isrc",
                    str(src),"src/core/TextKeyboard.cpp",
                    "src/services/NotificationService.cpp",
                    "-Wl,--gc-sections","-o",str(exe)],cwd=root,check=True)
    subprocess.run([str(exe)],cwd=root,check=True)
print("PASS browser keyboard integration routing and SD/USB notice contracts")
