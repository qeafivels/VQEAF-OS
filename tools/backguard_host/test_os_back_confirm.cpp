#include <cassert>
#include <iostream>
#include "../../src/core/OsBackConfirm.h"
static KeyEvent ev(Key k, bool pressed=true, bool lng=false, bool repeat=false) {
 return KeyEvent(k,pressed,lng,repeat);
}
int main() {
 int tests=0;
 auto check=[&](bool value){ ++tests; assert(value); };
 using R=OsBackConfirm::Result;
 OsBackConfirm m;
 check(!m.active());
 check(m.handle(ev(Key::Start))==R::Unhandled);
 check(m.destination()==ScreenId::Idle);
 const ScreenId apps[]={ScreenId::Snake,ScreenId::Browser,ScreenId::Music,
       ScreenId::Gallery,ScreenId::TextViewer,ScreenId::Shell,ScreenId::Notes,
       ScreenId::Calculator,ScreenId::Stopwatch};
 for(ScreenId from:apps){
   check(OsBackConfirm::eligible(from, ScreenId::Applications,ev(Key::A)));
   check(OsBackConfirm::eligible(from, ScreenId::Launcher,ev(Key::B)));
   check(OsBackConfirm::eligible(from, ScreenId::Collection,ev(Key::B)));
   check(OsBackConfirm::eligible(from, ScreenId::Files,ev(Key::A)));
   check(OsBackConfirm::eligible(from, ScreenId::Idle,ev(Key::A)));
   check(!OsBackConfirm::eligible(from,from,ev(Key::A))); // in-app back
   check(!OsBackConfirm::eligible(from,ScreenId::Applications,ev(Key::Menu)));
   check(!OsBackConfirm::eligible(from,ScreenId::Applications,ev(Key::A,false)));
   check(!OsBackConfirm::eligible(from,ScreenId::Applications,ev(Key::A,true,true)));
   check(!OsBackConfirm::eligible(from,ScreenId::Applications,ev(Key::A,true,false,true)));
   check(!OsBackConfirm::eligible(from,ScreenId::Recovery,ev(Key::A)));
 }
 const ScreenId unguarded[]={ScreenId::Splash,ScreenId::Idle,ScreenId::Lock,
   ScreenId::Launcher,ScreenId::Explorer,ScreenId::WiFi,ScreenId::BLE,
   ScreenId::Files,ScreenId::Themes,ScreenId::Settings,
   ScreenId::AppInstaller,ScreenId::Applications,ScreenId::Recovery,
   ScreenId::TaskSwitcher,ScreenId::QuickPanel,ScreenId::Notifications};
 for(ScreenId from:unguarded){
   check(!OsBackConfirm::eligible(from,ScreenId::Launcher,ev(Key::A)));
   check(!m.begin(from, ScreenId::Launcher,ev(Key::A)));
 }
 check(m.begin(ScreenId::Snake,ScreenId::Applications,ev(Key::A)));
 check(m.active());
 check(m.selected()==1);
 check(m.destination()==ScreenId::Applications);
 check(!m.begin(ScreenId::Browser,ScreenId::Launcher,ev(Key::B))); // no nested modal
 check(m.handle(ev(Key::Left))==R::Repaint);
 check(m.selected()==0);
 check(m.handle(ev(Key::Left))==R::Waiting);
 check(m.handle(ev(Key::Start,false))==R::Waiting);
 check(m.handle(ev(Key::Start,true,true))==R::Waiting);
 check(m.handle(ev(Key::Right))==R::Repaint);
 check(m.selected()==1);
 check(m.handle(ev(Key::Start))==R::Cancelled); // safe choice
 m.cancel();
 check(!m.active());
 check(m.selected()==1);
 check(m.begin(ScreenId::Browser,ScreenId::Launcher,ev(Key::B)));
 check(m.handle(ev(Key::Up))==R::Repaint);
 check(m.handle(ev(Key::Select))==R::Accepted);
 check(m.destination()==ScreenId::Launcher);
 m.cancel();
 check(m.begin(ScreenId::Snake,ScreenId::Applications,ev(Key::A)));
 check(m.handle(ev(Key::B))==R::Cancelled);
 m.cancel();
 check(m.begin(ScreenId::Snake,ScreenId::Applications,ev(Key::A)));
 check(m.handle(ev(Key::Menu))==R::Waiting); // modal consumes global shortcuts
 check(m.handle(ev(Key::Down))==R::Waiting);
 check(m.handle(ev(Key::Option))==R::Cancelled);
 m.cancel();
 check(!m.active());
 std::cout << "OsBackConfirm host checks PASS: " << tests << "\n";
 return 0;
}
