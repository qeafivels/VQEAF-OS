#pragma once
#include "Types.h"

// Core-only system Back confirmation.  It never owns a framebuffer or changes
// the VQEAF / Retro-Go renderer.  The caller draws with the EXISTING UI.dialog().
// No heap allocation, timers, callbacks, or blocking waits in this state machine.
class OsBackConfirm {
 public:
  enum class Result : uint8_t { Unhandled, Waiting, Repaint, Accepted, Cancelled };

  bool active() const { return active_; }
  int selected() const { return selected_; } // 0: Yes, 1: No (safe default)
  ScreenId destination() const { return destination_; }
  void cancel() { active_ = false; destination_ = ScreenId::Idle; selected_ = 1; }

  static bool eligible(ScreenId from, ScreenId to, const KeyEvent &e) {
    // Only an actual exit via a physical Back/soft-right press.  In-app Back
    // (dismiss a popup, leave preview, return to previous browser page) stays
    // with the app and retains its old behaviour.
    if (!e.pressed || e.longPress || e.repeat ||
        (e.key != Key::A && e.key != Key::B) || from == to) return false;
    if (!(to == ScreenId::Launcher || to == ScreenId::Applications ||
          to == ScreenId::Collection || to == ScreenId::Files ||
          to == ScreenId::Idle)) return false;
    switch (from) {
#if defined(VQEAF_ENABLE_LUA) && VQEAF_ENABLE_LUA
      case ScreenId::LuaApp:
#endif
      case ScreenId::Snake:
      case ScreenId::Browser:
      case ScreenId::Music:
      case ScreenId::Gallery:
      case ScreenId::TextViewer:
      case ScreenId::Shell:
      case ScreenId::Notes:
      case ScreenId::Calculator:
      case ScreenId::Stopwatch: return true;
      default: return false;
    }
  }

  bool begin(ScreenId from, ScreenId to, const KeyEvent &e) {
    if (active_ || !eligible(from, to, e)) return false;
    active_ = true;
    destination_ = to;
    selected_ = 1; // Never default to destroying the current app session.
    return true;
  }

  Result handle(const KeyEvent &e) {
    if (!active_) return Result::Unhandled;
    if (!e.pressed || e.longPress || e.repeat) return Result::Waiting;
    switch (e.key) {
      case Key::Left:
      case Key::Up:
        if (selected_ != 0) { selected_ = 0; return Result::Repaint; }
        return Result::Waiting;
      case Key::Right:
      case Key::Down:
        if (selected_ != 1) { selected_ = 1; return Result::Repaint; }
        return Result::Waiting;
      case Key::A:
      case Key::B:
      case Key::Option: return Result::Cancelled;
      case Key::Start:
      case Key::Select: return selected_ == 0 ? Result::Accepted : Result::Cancelled;
      default: return Result::Waiting;
    }
  }

 private:
  bool active_ = false;
  uint8_t selected_ = 1;
  ScreenId destination_ = ScreenId::Idle;
};
