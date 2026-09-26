#pragma once
#include "SketchpadModel.h"
#include "Apps.h"
// Standalone native app. Runs in stock OS without enabling unsigned Lua code.
// Data is confined to /Documents/Sketchpad/ on microSD, never to system NVS.
class SketchpadApp {
public:
  void enter() { editor_=false; confirm_=false; dirty_=false; message_=String(); }
  void draw(AppContext &ctx);
  ScreenId handle(AppContext &ctx,const KeyEvent &e);
private:
  SketchpadModel model_;
  uint8_t io_[SketchpadModel::MAX_BYTES]={0}; // static app member, NOT loopTask stack
  int selected_=0;
  bool editor_=false,confirm_=false,dirty_=false;
  int confirmChoice_=2;
  uint8_t tool_=SketchpadModel::Pen;
  int cx_=115,cy_=135;
  String message_;
  String filePath() const;
  bool load(AppContext &ctx);
  bool save(AppContext &ctx);
  bool importLegacy(AppContext &ctx);
  void paintCanvas(AppContext &ctx);
};
