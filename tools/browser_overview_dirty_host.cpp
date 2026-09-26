#include "../src/services/BrowserOverviewDirty.h"
#include <cassert>
#include <cstdio>
int main() {
  BrowserOverviewDirty d;
  using Kind=BrowserOverviewDirty::Kind;
  assert(d.update(false,0,1,0,0).kind==Kind::Full);
  assert(d.update(false,0,1,0,0).kind==Kind::None);
  assert(d.update(false,0,1,0,3).kind==Kind::Progress);
  auto moved=d.update(false,0,1,1,18);
  assert(moved.kind==Kind::Selection && moved.previousSelected==0);
  assert(d.update(false,0,1,1,18).kind==Kind::None);
  assert(d.update(false,1,1,5,34).kind==Kind::Full);
  assert(d.update(false,1,2,5,34).kind==Kind::Full);
  assert(d.update(true,1,2,5,34).kind==Kind::Full);
  d.invalidate();
  assert(d.update(false,1,2,5,34).kind==Kind::Full);
  std::puts("PASS allocation-free overview dirty repaint planner");
  return 0;
}
