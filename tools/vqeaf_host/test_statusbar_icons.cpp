// Raster regression of actual production SymbianUI.cpp with an opt-in
// connected WiFi host fixture. No hardware LCD or battery ADC claims.
#include "core/SymbianUI.h"
#include "core/UiLayoutGeometry.h"
#include "WiFi.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

namespace L=VqeafLayout;
static void testTheme(SymbianUI &ui,TFT_eSPI &tft,const std::string &root,
                      ThemeId theme,const char *label){
  ui.setTheme(theme);
  tft.fillScreen(0);
  ui.invalidateChrome();
  WiFi.mockStatus=0;
  WiFi.mockRssi=-50;
  ui.chrome("Header",false,false,false,false);
  const uint16_t ink=ui.c().chromeText;
  const uint16_t bg=ui.c().chrome;
  assert(L::STATUS_WIFI_X>=2*L::STATUS_ZONE_W);
  assert(L::STATUS_BATTERY.right()+L::STATUS_RIGHT_PAD<=L::W);
  assert(L::STATUS_WIFI.bottom()<L::TITLEBAR_H-2);
  assert(tft.get(L::STATUS_WIFI_X+1,L::STATUS_ICON_Y+15)==bg);
  assert(tft.get(L::STATUS_BATTERY_X,L::STATUS_ICON_Y+1)==ink);
  assert(tft.get(L::STATUS_BATTERY_X+17,L::STATUS_ICON_Y+7)==ink);
  auto before=tft.fb;
  const int fullBefore=tft.fullFills;
  WiFi.mockStatus=WL_CONNECTED;
  ui.chrome("Header",true,false,false,false);
  assert(tft.fullFills==fullBefore);
  // 4 ascending 3px bars: first ends at y+15; fourth begins at y.
  assert(tft.get(L::STATUS_WIFI_X+1,L::STATUS_ICON_Y+15)==ink);
  assert(tft.get(L::STATUS_WIFI_X+13,L::STATUS_ICON_Y)==ink);
  assert(tft.get(L::STATUS_WIFI_X+13,L::STATUS_ICON_Y+15)==ink);
  assert(tft.get(L::STATUS_BATTERY_X+17,L::STATUS_ICON_Y+7)==ink);
  int changed=0;
  for(int y=0;y<L::H;++y)for(int x=0;x<L::W;++x){
    if(before[y*L::W+x]!=tft.fb[y*L::W+x]){
      assert(x>=2*L::STATUS_ZONE_W&&x<L::W);
      assert(y>=0&&y<L::TITLEBAR_H-2);
      ++changed;
    }
  }
  assert(changed>=35);
  tft.savePPM((root+std::string("statusbar_large_")+label+".ppm").c_str());

  // Lower signal must erase stale bars while preserving battery and clock.
  before=tft.fb;
  WiFi.mockRssi=-85;
  ui.chrome("Header",true,false,false,false);
  assert(tft.get(L::STATUS_WIFI_X+1,L::STATUS_ICON_Y+15)==ink);
  assert(tft.get(L::STATUS_WIFI_X+13,L::STATUS_ICON_Y)==bg);
  assert(tft.get(L::STATUS_BATTERY_X+17,L::STATUS_ICON_Y+7)==ink);
  assert(tft.fullFills==fullBefore);
  for(int y=0;y<L::H;++y)for(int x=0;x<L::W;++x)
    if(before[y*L::W+x]!=tft.fb[y*L::W+x]){
      assert(x>=2*L::STATUS_ZONE_W&&x<L::W);
      assert(y<L::TITLEBAR_H-2);
    }
  tft.savePPM((root+std::string("statusbar_low_")+label+".ppm").c_str());
}
int main(int argc,char **argv){
 assert(argc>=2);
 TFT_eSPI tft;
 SymbianUI ui(tft);
 testTheme(ui,tft,argv[1],ThemeId::S60Green,"green");
 testTheme(ui,tft,argv[1],ThemeId::Classic,"night");
 puts("PASS: 18px status icons fit 27px chrome; pixel geometry, zero full-fill, RSSI stale-bar clearing, two themes");
}
