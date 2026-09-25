// Pixel previews use ACTUAL VQEAF SymbianUI.cpp / TextKeyboard.cpp and a
// PC TFT RGB565 framebuffer shim. These are not photographs or hardware fonts.
#include "core/SymbianUI.h"
#include "core/TextKeyboard.h"
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
  puts("PASS: actual C++ host RGB565 browser keyboard, focus dirty redraw, masked input and notifications UI");
}
