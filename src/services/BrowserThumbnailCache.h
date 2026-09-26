#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

// 2 x 64x48 RGB565 PSRAM LRU plus CRC-checked persistent LittleFS tier.
// Native browser retains ownership of its LCD, WiFi and SD. This component
// owns neither drivers nor a second framebuffer.
class BrowserThumbnailCache {
public:
  static constexpr int W=64,H=48;
  bool begin();
  ~BrowserThumbnailCache();
  bool ready() const {return initialized;}
  bool has(const char *url);
  bool draw(TFT_eSPI &tft,const char *url,int x,int y);
  // Explicit bounded prefetch from main OS task, never inside screen redraw.
  bool prefetch(const char *url);
  int ramHits() const {return hitsRam;}
  int flashHits() const {return hitsFlash;}
  int failedRequests() const {return failures;}
private:
  struct Tile {
    uint64_t key=0;
    uint16_t *pixels=nullptr;
    uint32_t touched=0;
    bool valid=false;
  };
  Tile slots[2];
  bool initialized=false;
  bool flashReady=false;
  uint32_t accessCounter=1;
  int hitsRam=0,hitsFlash=0,failures=0;
  Tile *find(uint64_t key);
  Tile *victim();
  bool loadFlash(uint64_t key,Tile &tile);
  void saveFlash(uint64_t key,const Tile &tile);
  static uint64_t hashUrl(const char *url);
  static uint32_t crc32(const uint8_t *data,size_t len);
  static bool fetchAndDecode(const char *url,uint16_t *pixels);
};