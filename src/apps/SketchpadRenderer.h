#pragma once
// Exactly the same bounded RGB565 painter is used on ESP32-S3 and by the
// host screenshot/soak suite. This draws the existing 3-page native Sketchpad,
// not a mockup or a separate Python imitation of its graphics.
#include "SketchpadModel.h"
#include "../core/UiLayoutGeometry.h"
#include <TFT_eSPI.h>

namespace SketchpadRenderer {
static constexpr uint16_t PAPER=0xFFF7;
static constexpr uint16_t RULED=0xBEDC;
static constexpr uint16_t MARGIN=0xDAE8;
static constexpr uint16_t INK=0x124E;
static constexpr uint16_t GRAPHITE=0x630C;
static constexpr uint16_t CURSOR=0xF800;
inline void paint(TFT_eSPI &d,const SketchpadModel &model,int selected,int tool,
                  int cx,int cy,const String &message){
  static const char *const toolNames[]={"Ink","Pencil","Eraser"};
  if(tool<0 || tool>2)tool=0;
  d.fillRect(0,VqeafLayout::CONTENT_TOP,240,
             VqeafLayout::FOOTER_Y-VqeafLayout::CONTENT_TOP,PAPER);
  for(int y=58;y<279;y+=20)d.drawFastHLine(0,y,240,RULED);
  d.drawFastVLine(23,49,230,MARGIN);
  d.drawFastVLine(25,49,230,MARGIN);
  for(int i=0;i<model.count();++i){
    const SketchpadModel::Stroke &s=model.at(i);
    const uint16_t ink=s.tool==SketchpadModel::Eraser?PAPER:
                       s.tool==SketchpadModel::Pencil?GRAPHITE:INK;
    d.drawLine(s.x0,s.y0,s.x1,s.y1,ink);
  }
  d.drawRect(cx-3,cy-3,7,7,CURSOR);
  d.setTextColor(INK,PAPER);d.setTextFont(1);d.setTextSize(1);
  d.setCursor(32,34);
  d.print(String("P")+String(selected+1)+" "+toolNames[tool]+
          "  "+model.count()+"/"+SketchpadModel::MAX_STROKES);
  // The UI's real bottom softkeys already show Tool/Save/Back. Never write
  // hints into the footer or overdraw the final canvas row.
  if(message.length()){
    d.setCursor(33,268);
    d.print(message.substring(0,30));
  }
}
} // namespace SketchpadRenderer
