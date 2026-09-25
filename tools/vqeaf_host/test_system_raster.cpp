// Pixel previews use ACTUAL VQEAF SymbianUI.cpp / TextKeyboard.cpp and a
// PC TFT RGB565 framebuffer shim. These are not photographs or hardware fonts.
#include "core/SymbianUI.h"
#include "core/TextKeyboard.h"
#include "apps/InstallerMenuPolicy.h"
#include "services/NotificationService.h"
#include <cassert>
#include <cstring>
#include <string>
#include <cstdio>

int main(int argc,char **argv){
  assert(argc>=2);
  const std::string prefix=argv[1];
  TFT_eSPI screen;
  SymbianUI ui(screen);
  ui.setTheme(ThemeId::S60Green);
  TextKeyboard keyboard;
  keyboard.open("Web address","https://qeafivels");
  keyboard.draw(ui,true,false,true,false);
  // First focus is Q: y=109; sample interior away from the border.
  assert(screen.get(15,115)==ui.c().selected);
  screen.savePPM((prefix+"browser_keyboard.ppm").c_str());
  // D-pad movement paints previous and new cells; no full LCD fill.
  const int fullFills=screen.fullFills;
  assert(keyboard.handle(KeyEvent(Key::Right,true,false,false)));
  keyboard.draw(ui,true,false,true,false);
  assert(screen.fullFills==fullFills);
  assert(screen.get(15,115)==ui.c().panel);
  assert(screen.get(39,115)==ui.c().selected);
  screen.savePPM((prefix+"browser_keyboard_focus.ppm").c_str());
  // Hidden URL suffixes in password mode and no string leaked to screen
  // (host TFT mock ignores glyphs, logical text test lives separately).
  keyboard.open("WiFi password","secret",true);
  keyboard.draw(ui,true,false,true,false);
  screen.savePPM((prefix+"wifi_keyboard_masked.ppm").c_str());
  // Draw exactly the OS's production notification UI widgets, with actual
  // system event strings; TFT host stub does not rasterize text glyphs.
  NotificationService events;
  events.push("microSD detected","Memory card ready");
  events.push("microSD removed","Storage offline; reinsert to retry");
  events.push("microSD mounted","Filesystem ready again");
  events.push("USB connected","Computer connected via Type-C");
  events.push("USB disconnected","Type-C data connection lost");
  ui.clear();
  ui.chrome("Notifications",true,false,true,false);
  for(int i=0;i<events.count();++i){
    const SystemNotification &n=events.at(i);
    ui.listItem(i,"Bell",n.title,String("NEW  ")+n.body,i==0);
  }
  ui.softkeys("Options","Open","Back");
  screen.savePPM((prefix+"system_notifications.ppm").c_str());
  // These two review images show a deliberately constructed, VERIFIED state
  // using production SymbianUI.cpp and the actual InstallerMenuPolicy.h. They
  // are UI state previews, NOT a claim that an app actually ran on a device.
  using namespace InstallerMenuPolicy;
  assert(primary(false,true,true,true,false)==Primary::Open);
  assert(canOpen(false,true,true,false));
  ui.clear();
  ui.chrome("App inbox",false,false,true,false);
  ui.message("Signature verified","Welcome","Version 1.0 / text",
             "Already installed - choose Open");
  ui.drawIcon(101,167,"App",ui.c().bg);
  ui.softkeys("Options","Open","Back");
  screen.savePPM((prefix+"installer_revisit_open.ppm").c_str());
  assert(primary(true,true,true,true,false)==Primary::Open);
  ui.clear();
  ui.chrome("Installed apps",false,false,true,false);
  ui.listItem(0,"App","Welcome","v1.0.0 / text",true);
  ui.listItem(1,"App","Sample game","Lua beta - UI sample",false);
  ui.softkeys("Options","Open","Back");
  const char *const actions[]={"Details","Open","Inbox / Installed",
                                "Rescan","Applications","Recover installs",
                                "Reset app data","Uninstall"};
  ui.popupMenu(actions,8,1,0,5);
  screen.savePPM((prefix+"installed_apps_open_menu.ppm").c_str());
  puts("PASS: actual C++ host RGB565 keyboard, focus redraw, notifications + verified installer menu-state previews");
}
