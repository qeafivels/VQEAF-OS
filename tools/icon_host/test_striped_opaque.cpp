#include "VqeafIconRenderer.h"
#include "TFT_eSPI.h"
#include <cassert>
#include <cstdio>
int main() {
 const uint16_t bg[] = {0x0000,0xFFFF,0x3C67,0xD123};
 size_t scenarios=0;
 for (unsigned s : {24U,36U}) {
  for (unsigned id=0;id<12;++id) for (auto color:bg) {
   TFT_eSPI reference,striped;
   const auto icon=static_cast<VqeafIcons::Id>(id);
   assert(VqeafIcons::draw(reference,icon,25,31,uint8_t(s),color,
                          VqeafIcons::Palette::standard(),true));
   assert(VqeafIcons::drawOpaque(striped,icon,25,31,uint8_t(s),color));
   assert(reference.fb==striped.fb);
   assert(striped.imageCalls==s/4);
   assert(striped.imagePixels==s*s);
   assert(striped.beginWrites==1 && striped.endWrites==1);
   assert(striped.swapped==false);
   ++scenarios;
  }
 }
 TFT_eSPI bad;
 assert(!VqeafIcons::drawOpaque(bad,VqeafIcons::Id::Count,0,0,24,0));
 assert(!VqeafIcons::drawOpaque(bad,VqeafIcons::Id::WiFi,0,0,25,0));
 assert(bad.imageCalls==0);
 std::printf("PASS %zu pixel-perfect opaque icon cases; stripe transfer count 6/9\n",scenarios);
}
