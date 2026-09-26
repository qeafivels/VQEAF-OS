#pragma once
#include <stdint.h>

// Small deterministic fixed-point scroll/overview controller.
// No heap, float, framebuffer, or hardware dependencies.
class BrowserMotion {
public:
  static constexpr int VIEWPORT = 208;
  static constexpr int ROW_HEIGHT = 16;
  void reset(int lines) {
    docPx = lines > 0 ? lines * ROW_HEIGHT : 0;
    targetQ8 = visualQ8 = velocityQ8 = 0;
    zoomLevel = 1; overviewMode = false; lastTick = 0;
  }
  void setDocumentLines(int lines) {
    docPx = lines > 0 ? lines * ROW_HEIGHT : 0;
    targetQ8 = clamp(targetQ8); visualQ8 = clamp(visualQ8);
  }
  void jumpToLine(int line) {
    targetQ8 = clamp(line * ROW_HEIGHT * 256);
    velocityQ8 = 0;
  }
  void scrollPixels(int pixels) {
    targetQ8 = clamp(targetQ8 + pixels * 256);
    velocityQ8 = pixels * 128;
  }
  void release() {
    // Fixed-point drag-release coasting capped to avoid moving focus offscreen.
    if (velocityQ8 > 8 * 256) velocityQ8 = 8 * 256;
    if (velocityQ8 < -8 * 256) velocityQ8 = -8 * 256;
  }
  bool tick(uint32_t now) {
    if (!lastTick) {lastTick=now;return false;}
    if ((uint32_t)(now-lastTick)<17U) return false;
    lastTick=now;
    const int old = pixel();
    if (velocityQ8) {
      targetQ8 = clamp(targetQ8 + velocityQ8);
      velocityQ8 = velocityQ8 * 230 / 256;
      if (velocityQ8 < 16 && velocityQ8 > -16) velocityQ8=0;
    }
    int delta=targetQ8-visualQ8;
    visualQ8+=delta / 3;
    if (delta < 256 && delta > -256) visualQ8=targetQ8;
    return old!=pixel();
  }
  void toggleOverview() {overviewMode=!overviewMode;velocityQ8=0;}
  bool overview() const {return overviewMode;}
  void zoom(int direction) {
    int v=int(zoomLevel)+direction;
    if(v<1)v=1;if(v>8)v=8;zoomLevel=(uint8_t)v;
  }
  int zoomValue() const {return zoomLevel;}
  int pixel() const {return visualQ8 >> 8;}
  int targetPixel() const {return targetQ8 >> 8;}
  int firstLine() const {return pixel()/ROW_HEIGHT;}
  int linePixelOffset() const {return pixel()%ROW_HEIGHT;}
  int documentPixels() const {return docPx;}
  int maxScroll() const {return maxPx();}
  void overviewPan(int dir) {
    // Pan scales with zoom; do not allocate enlarged page images.
    scrollPixels(dir * (VIEWPORT * zoomLevel / 4));
    velocityQ8=0;
  }
  // Draw miniature document at 1/zoomLevel, choose a viewport-sized window.
  int miniatureHeight() const {return (docPx+zoomLevel-1)/zoomLevel;}
  int overviewCursorY() const {return targetPixel()/zoomLevel;}
  int overviewCursorHeight() const {return (VIEWPORT+zoomLevel-1)/zoomLevel;}
private:
  int32_t visualQ8=0,targetQ8=0,velocityQ8=0;
  int docPx=0;
  uint8_t zoomLevel=1;bool overviewMode=false;
  uint32_t lastTick=0;
  int maxPx() const {return docPx>VIEWPORT?docPx-VIEWPORT:0;}
  int32_t clamp(int32_t v) const {
    if(v<0)return 0;
    const int32_t maxQ8=maxPx()*256;
    return v>maxQ8?maxQ8:v;
  }
};