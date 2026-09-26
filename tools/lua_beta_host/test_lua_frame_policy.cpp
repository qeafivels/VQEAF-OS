#include "QeLuaFramePolicy.h"
#include <stdint.h>
#include <cstdio>
#include <vector>

static bool ensure(bool cond, const char *what) {
  if (!cond) std::fprintf(stderr, "FAIL %s\n", what);
  return cond;
}
int main() {
  QeLuaFramePolicy gate;
  std::vector<uint16_t> draw(QeLuaFramePolicy::kPixels, 0);
  std::vector<uint16_t> last(QeLuaFramePolicy::kPixels, 0);
  if (!ensure(QeLuaFramePolicy::kBytes == 129600, "viewport must exclude OS chrome")) return 1;
  if (!ensure(gate.needsPresent(draw.data(), last.data()), "first frame is mandatory")) return 2;
  last = draw; gate.markPresented();
  if (!ensure(!gate.needsPresent(draw.data(), last.data()), "unchanged full clear skipped")) return 3;
  draw[3 + 12*QeLuaFramePolicy::kWidth] = 0xf800;
  if (!ensure(gate.needsPresent(draw.data(), last.data()), "single changed pixel flushes")) return 4;
  last = draw; gate.markPresented();
  if (!ensure(!gate.needsPresent(draw.data(), last.data()), "no redundant LCD transfer")) return 5;
  // The Back modal draws on the physical LCD only, not the queued sprite.
  gate.invalidate();
  if (!ensure(gate.needsPresent(draw.data(), last.data()), "modal cancellation restores occluded area")) return 6;
  gate.markPresented();
  if (!ensure(!gate.needsPresent(draw.data(), last.data()), "restore exactly once")) return 7;
  if (!ensure(!gate.needsPresent(nullptr, last.data()) && !gate.needsPresent(draw.data(), nullptr),
              "missing PSRAM buffers fail closed")) return 8;
  // Regression: repeated engine.clear() followed by identical primitives must
  // not transfer the unchanged 129600-byte frame 120 times.
  unsigned presents=0, skipped=0;
  for (int tick=0;tick<120;++tick) {
    std::fill(draw.begin(),draw.end(),0);
    draw[12*QeLuaFramePolicy::kWidth + 3]=0xf800;
    if (gate.needsPresent(draw.data(),last.data())) {
      ++presents;
      last=draw;
      gate.markPresented();
    } else ++skipped;
  }
  if (!ensure(presents==0 && skipped==120,"120 identical script clears transfer zero frames")) return 9;
  draw[12*QeLuaFramePolicy::kWidth+3]=0;
  draw[12*QeLuaFramePolicy::kWidth+4]=0xf800;
  if (!ensure(gate.needsPresent(draw.data(),last.data()),"moving sprite changes final image")) return 10;
  // SPI crop planner: first present/invalidation must restore full viewport;
  // local edits must transfer only affected scanlines, no new framebuffer.
  gate.invalidate();
  auto s=gate.stripe(draw.data(),last.data());
  if (!ensure(s.full && s.first==0 && s.rows==270,"invalidated frame is full")) return 11;
  gate.markPresented();
  last=draw;
  s=gate.stripe(draw.data(),last.data());
  if (!ensure(!s.full && s.rows==0,"unchanged frame has no crop")) return 12;
  draw[10*QeLuaFramePolicy::kWidth+1]=0x4321;
  draw[13*QeLuaFramePolicy::kWidth+2]=0x2345;
  s=gate.stripe(draw.data(),last.data());
  if (!ensure(!s.full && s.first==10 && s.rows==4,"local edit cropped to four rows")) return 13;
  last=draw;
  draw[0]=0xFFFF;
  draw[269*QeLuaFramePolicy::kWidth]=0xFFFF;
  s=gate.stripe(draw.data(),last.data());
  if (!ensure(s.full && s.rows==270,"distant edits require full frame")) return 14;
  if (!ensure(gate.stripe(nullptr,last.data()).full,"missing buffer cannot crop")) return 15;
  draw=last;
  draw[20*QeLuaFramePolicy::kWidth]=0x1111;
  draw[99*QeLuaFramePolicy::kWidth]=0x1111;
  s=gate.stripe(draw.data(),last.data());
  if (!ensure(!s.full && s.first==20 && s.rows==80,"80-row dirty stripe boundary")) return 16;
  draw[100*QeLuaFramePolicy::kWidth]=0x1111;
  s=gate.stripe(draw.data(),last.data());
  if (!ensure(s.full && s.rows==270,"81-row stripe uses full frame")) return 17;
  std::puts("PASS 17 Lua frame-policy checks (including 120 unchanged simulated frames)");
}
