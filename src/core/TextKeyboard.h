#pragma once
#include <Arduino.h>
#include "SymbianUI.h"
#include "Types.h"

// One OS-wide virtual keyboard, modeled on Qeafbrowser's four-row keypad.
// No heap allocations for layout; caller-owned String stays bounded.
class TextKeyboard {
public:
  void open(const String &caption, const String &initial = "", bool password = false);
  bool active() const { return shown; }
  bool handle(const KeyEvent &e);
  void draw(SymbianUI &ui, bool wifi, bool ble, bool sd, bool hour12);
  bool accepted() const { return done; }
  bool cancelled() const { return cancel; }
  String value() const { return text; }
private:
  bool shown=false,done=false,cancel=false,masked=false,upper=false,symbols=false;
  String caption,text;
  int row=1,col=0;
  Key lastNumeric=Key::None;
  int numericTap=0;
  uint32_t numericAt=0;
  static constexpr int MAX_TEXT=159;
  static constexpr int LETTER_ROWS=4;
  static constexpr int SPECIAL_ROW=4;
  static constexpr int SPECIAL_COUNT=5;
  const char *rowChars(int index) const;
  int colsFor(int index) const;
  bool typeDigit(Key digit);
  bool insert(const char *s);
  void activate();
  void moveRow(int direction);
};