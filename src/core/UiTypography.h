#pragma once
#include <stdint.h>
// Central UI text roles for the ST7789 native 240x320 portrait panel.
// Midnight uses unified regular/bold DejaVu-derived, pre-rasterized ASCII and
// Vietnamese glyphs in UiVietnameseFont.h. Previous skins retain their legacy
// TFT_eSPI raster for ASCII. No TTF engine or frame-time allocation.
namespace UiTypography {
  // Midnight type scale: 11px regular / 13px bold / 26px clock.
  // All variants share one compact DejaVu-derived NFC raster family.
  static constexpr uint8_t MICRO=1;   // regular: status details, metadata
  static constexpr uint8_t CAPTION=1; // regular: centered launcher labels
  static constexpr uint8_t BODY=2;    // bold: list primary text / softkeys
  static constexpr uint8_t TITLE=2;   // bold: section and header titles
  static constexpr uint8_t CLOCK_SCALE=2;
  static constexpr uint8_t REGULAR_GLYPH_H=18;
  static constexpr uint8_t BOLD_GLYPH_H=18;
  static constexpr uint8_t LIST_TITLE_OFFSET_Y=3;
  static constexpr uint8_t LIST_DETAIL_OFFSET_Y=22;
  // Keep text inside the immutable 42px list row and 22px footer.
  static constexpr int MENU_CAPTION_Y=44;
  static constexpr int HOME_CLOCK_Y=53;
  static constexpr int HOME_DATE_Y=91;
  static constexpr int HOME_HINT_Y=272;
  static constexpr int HEADER_TITLE_X=4;
  static constexpr int HEADER_TITLE_Y=5;
  static constexpr int FOOTER_TEXT_Y=301;
}
