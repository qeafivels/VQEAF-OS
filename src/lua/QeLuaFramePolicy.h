#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Host-testable presentation gate. A Lua callback paints an off-screen RGB565
// sprite; only a complete, changed frame can be submitted to the physical LCD.
// No framebuffer content is trusted beyond this caller-owned lifetime.
class QeLuaFramePolicy {
public:
  static constexpr size_t kWidth = 240;
  static constexpr size_t kHeight = 270;
  static constexpr size_t kPixels = kWidth * kHeight;
  static constexpr size_t kBytes = kPixels * sizeof(uint16_t);

  bool needsPresent(const uint16_t *drawing, const uint16_t *previous) const {
    return drawing && previous &&
           (!presented_ || memcmp(drawing, previous, kBytes) != 0);
  }
  // Bounded dirty-row planner. One full-width stripe minimizes TFT address
  // windows without copying pixels to internal RAM. A changed stripe taller
  // than 80 rows uses the regular full-frame transfer instead.
  struct Stripe { uint16_t first; uint16_t rows; bool full; };
  Stripe stripe(const uint16_t *drawing, const uint16_t *previous) const {
    if (!drawing || !previous || !presented_)
      return {0, static_cast<uint16_t>(kHeight), true};
    const size_t rowBytes = kWidth * sizeof(uint16_t);
    size_t first = 0;
    while (first < kHeight &&
           memcmp(drawing + first*kWidth, previous + first*kWidth, rowBytes) == 0)
      ++first;
    if (first == kHeight) return {0, 0, false};
    size_t last = kHeight - 1;
    while (last > first &&
           memcmp(drawing + last*kWidth, previous + last*kWidth, rowBytes) == 0)
      --last;
    const size_t rows = last - first + 1;
    return rows <= 80
        ? Stripe{static_cast<uint16_t>(first),static_cast<uint16_t>(rows),false}
        : Stripe{0,static_cast<uint16_t>(kHeight),true};
  }
  void markPresented() { presented_ = true; }
  // An OS dialog covers some viewport pixels without touching the sprite.
  // Invalidate on dismiss so the unchanged sprite restores the entire area.
  void invalidate() { presented_ = false; }
  bool hasPresented() const { return presented_; }
private:
  bool presented_ = false;
};
