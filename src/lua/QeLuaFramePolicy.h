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
  void markPresented() { presented_ = true; }
  // An OS dialog covers some viewport pixels without touching the sprite.
  // Invalidate on dismiss so the unchanged sprite restores the entire area.
  void invalidate() { presented_ = false; }
  bool hasPresented() const { return presented_; }
private:
  bool presented_ = false;
};
