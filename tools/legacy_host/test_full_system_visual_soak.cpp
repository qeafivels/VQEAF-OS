// C++11 host raster/stress of real OS chrome and the exact native Sketchpad
// painter. HOST performance is NOT an ESP32-S3 frequency/FPS measurement.
#include "apps/SketchpadRenderer.h"
#include "core/SymbianUI.h"
#include "core/UiFrameMetrics.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <cstring>
#include <string>

static const char *root=nullptr;
static void capture(TFT_eSPI &tft,const char *stem) {
  std::string path=std::string(root)+stem+".ppm";
  tft.savePPM(path.c_str());
}
static void chrome(SymbianUI &ui,ThemeId theme,int page) {
  ui.setTheme(theme);
  ui.invalidateChrome();
  ui.chrome(String("Sketchpad P")+String(page),true,false,true,false);
}
int main(int argc,char **argv){
  assert(argc==2);root=argv[1];
  TFT_eSPI tft;
  SymbianUI ui(tft);
  SketchpadModel p;
  static uint8_t buffer[SketchpadModel::MAX_BYTES];
  assert(p.count()==0);
  chrome(ui,ThemeId::S60Green,1);
  SketchpadRenderer::paint(tft,p,0,SketchpadModel::Pen,115,139,String());
  ui.softkeys("Tool","Save","Back");
  capture(tft,"sketch_01_empty_green");
  assert(tft.get(175,65)==SketchpadRenderer::PAPER);
  assert(tft.get(115-3,139-3)==SketchpadRenderer::CURSOR);

  // Strictly bounded real strokes, different ink/graphite/eraser and cursor.
  assert(p.append(60,130,154,130,SketchpadModel::Pen));
  assert(p.append(154,130,108,83,SketchpadModel::Pen));
  assert(p.append(108,83,60,130,SketchpadModel::Pen));
  assert(p.append(76,130,76,188,SketchpadModel::Pencil));
  assert(p.append(141,130,141,188,SketchpadModel::Pencil));
  assert(p.append(76,188,141,188,SketchpadModel::Pencil));
  assert(p.append(101,166,115,166,SketchpadModel::Eraser));
  chrome(ui,ThemeId::S60Green,1);
  SketchpadRenderer::paint(tft,p,0,SketchpadModel::Pen,175,209,String("Saved to microSD"));
  ui.softkeys("Tool","Save","Back");
  capture(tft,"sketch_02_strokes_green");
  assert(tft.get(61,130)==SketchpadRenderer::INK);
  assert(tft.get(77,165)==SketchpadRenderer::PAPER);
  assert(tft.get(76,165)==SketchpadRenderer::GRAPHITE);
  assert(tft.get(107,166)==SketchpadRenderer::PAPER);
  assert(tft.get(175-3,209-3)==SketchpadRenderer::CURSOR);

  size_t used=p.encode(buffer,sizeof buffer);
  SketchpadModel loaded;
  assert(used && loaded.decode(buffer,used));
  assert(loaded.count()==p.count());
  // Dark theme tests chrome/theme changes without recoloring notebook paper.
  chrome(ui,ThemeId::Classic,2);
  SketchpadRenderer::paint(tft,loaded,1,SketchpadModel::Pencil,185,219,String());
  ui.softkeys("Tool","Save","Back");
  capture(tft,"sketch_03_loaded_night");
  assert(tft.get(61,130)==SketchpadRenderer::INK);

  // Eraser screenshot and full redraw removes stale cursor and ink segments.
  assert(loaded.append(60,130,154,130,SketchpadModel::Eraser));
  chrome(ui,ThemeId::Classic,3);
  SketchpadRenderer::paint(tft,loaded,2,SketchpadModel::Eraser,185,210,String("Page 3: pencil/eraser"));
  ui.softkeys("Tool","Save","Back");
  capture(tft,"sketch_04_eraser_night");
  assert(tft.get(100,130)==SketchpadRenderer::PAPER);
  assert(tft.get(108,83)==SketchpadRenderer::INK);

  // Real model stress with 20,000 deterministic open-edit-undo-save-reload
  // transitions, including erased/corrupt payload rejection.  No filesystem
  // is touched here: SD power-loss/hotplug belongs to storage mock/board tests.
  using clock=std::chrono::steady_clock;
  const auto start=clock::now();
  SketchpadModel state;
  unsigned edits=0,undos=0,validRestores=0,badRejected=0;
  for(int i=0;i<20000;++i) {
    int x=32+i%162,y=70+i%155;
    if(!state.append(x,y,x+1,y+1,uint8_t(i%3))) {
      assert(state.count()==SketchpadModel::MAX_STROKES);
      state.clear();
      assert(state.append(x,y,x+1,y+1,uint8_t(i%3)));
    }
    ++edits;
    if(i%5==0){assert(state.undo());++undos;}
    const size_t n=state.encode(buffer,sizeof buffer);
    assert(n>=SketchpadModel::HEADER_BYTES);
    if(i%4==0) {
      SketchpadModel roundtrip;
      assert(roundtrip.decode(buffer,n));
      assert(roundtrip.count()==state.count());
      ++validRestores;
    }
    if(i%11==0) {
      const unsigned at=unsigned(SketchpadModel::HEADER_BYTES+(n-SketchpadModel::HEADER_BYTES)/2);
      buffer[at]^=uint8_t(1u+(i%127));
      SketchpadModel lastValid=state;
      assert(!lastValid.decode(buffer,n));
      assert(lastValid.count()==state.count());
      buffer[at]^=uint8_t(1u+(i%127));
      ++badRejected;
    }
  }
  const auto modelEnd=clock::now();
  const double modelS=std::chrono::duration<double>(modelEnd-start).count();
  // Do not label HOST wall time as CPU frequency/FPS of the real board.
  std::vector<double> redrawMs;
  redrawMs.reserve(240);
  const int fullStart=tft.fullFills;
  for(int i=0;i<240;++i){
    const auto t0=clock::now();
    ui.chrome("Sketchpad",true,false,true,false);
    SketchpadRenderer::paint(tft,state,0,i%3,135+i%60,150,String());
    ui.softkeys("Tool","Save","Back");
    const auto t1=clock::now();
    redrawMs.push_back(std::chrono::duration<double,std::milli>(t1-t0).count());
  }
  assert(tft.fullFills==fullStart); // No accidental fillScreen of LCD.
  std::sort(redrawMs.begin(),redrawMs.end());
  const auto t2=clock::now();
  const double redrawS=std::chrono::duration<double>(t2-modelEnd).count();
  assert(tft.fb.size()==240*320 && edits==20000 && validRestores==5000);
  std::printf("HOST_STRESS edits=%u undo=%u save_restore=%u corrupt_rejected=%u model_seconds=%.4f redraws=240 redraw_seconds=%.4f redraw_median_ms=%.4f redraw_p95_ms=%.4f full_screen_fills=%d\n",
    edits,undos,validRestores,badRejected,modelS,redrawS,redrawMs[120],redrawMs[227],tft.fullFills-fullStart);
  puts("PASS: C++ production sketch painter, S60/Night graphics, 240 redraws, 20k edit cycles, corrupt payload rejection");
}
