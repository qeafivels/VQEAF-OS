#include "SketchpadApp.h"
#include <esp_heap_caps.h>
// Original implementation inspired by legacy E524546 Paint/Notes behavior.
// No source from its vendor Lua runtime is linked into VQEAF-OS.
namespace {
const uint16_t PAPER=0xFFF7, RULED=0xBEDC, MARGIN=0xDAE8;
const uint16_t INK=0x124E, GRAPHITE=0x630C, CURSOR=0xF800;
const char *const TOOLS[]={"Ink","Pencil","Eraser"};
const char *const ROOT="/Documents/Sketchpad";
}
String SketchpadApp::filePath() const {
  return String(ROOT)+"/page"+String(selected_+1)+".qsk";
}
bool SketchpadApp::load(AppContext &ctx){
  model_.clear();dirty_=false;
  if(!ctx.storage.mounted()){message_="Insert microSD";return false;}
  const String path=filePath();
  if(!ctx.storage.recoverAtomicFile(path)){message_="SD recovery failed";return false;}
  if(!ctx.storage.exists(path)){message_="New page";return true;}
  File f=ctx.storage.fs().open(path,FILE_READ);
  if(!f){message_="Cannot open saved page";return false;}
  const size_t bytes=(size_t)f.size();
  const bool ok=bytes>=SketchpadModel::HEADER_BYTES &&
                bytes<=sizeof(io_) && f.read(io_,bytes)==(int)bytes;
  f.close();
  if(!ok || !model_.decode(io_,bytes)){
    message_="Page damaged - not overwritten";
    model_.clear();return false;
  }
  message_=String("Loaded ")+model_.count()+" strokes";
  return true;
}
bool SketchpadApp::save(AppContext &ctx){
  if(!ctx.storage.mounted()){message_="microSD not mounted";return false;}
  if(!ctx.storage.ensureDir(ROOT)){message_="Cannot create Sketchpad folder";return false;}
  const size_t bytes=model_.encode(io_,sizeof(io_));
  if(!bytes || !ctx.storage.writeAtomic(filePath(),io_,bytes)){
    message_="Write failed; drawing still in RAM";
    return false;
  }
  dirty_=false;message_="Saved to microSD";
  ctx.notifications.push("Sketchpad",String("Page ")+String(selected_+1)+" saved");
  return true;
}
bool SketchpadApp::importLegacy(AppContext &ctx){
  if(!ctx.storage.mounted()){message_="Insert microSD";return false;}
  const String dest=filePath();
  if(!ctx.storage.recoverAtomicFile(dest)){message_="SD recovery failed";return false;}
  if(ctx.storage.exists(dest)){message_="Page exists: import blocked";return false;}
  const String src=String(ROOT)+"/legacy/trang"+String(selected_+1)+".dat";
  File f=ctx.storage.fs().open(src,FILE_READ);
  if(!f){message_="Copy legacy trangN.dat to /legacy/";return false;}
  const size_t size=(size_t)f.size();
  if(!size || size>8192){f.close();message_="Legacy file too large or empty";return false;}
  char *buffer=(char*)heap_caps_malloc(size+1,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  if(!buffer){f.close();message_="Not enough PSRAM";return false;}
  const bool read=f.read((uint8_t*)buffer,size)==(int)size;
  f.close();
  if(read)buffer[size]='\0';
  const bool parsed=read && model_.importLegacy(buffer,size);
  heap_caps_free(buffer);
  if(!parsed){message_="Invalid legacy page";return false;}
  dirty_=true;
  if(!save(ctx)){model_.clear();dirty_=false;return false;}
  message_="Legacy page imported; source intact";
  return true;
}
void SketchpadApp::paintCanvas(AppContext &ctx){
  TFT_eSPI &d=ctx.ui.display();
  d.fillRect(0,SymbianUI::CONTENT_TOP,240,
             SymbianUI::SOFTKEY_TOP-SymbianUI::CONTENT_TOP,PAPER);
  for(int y=58;y<279;y+=20)d.drawFastHLine(0,y,240,RULED);
  d.drawFastVLine(23,49,230,MARGIN);
  d.drawFastVLine(25,49,230,MARGIN);
  for(int i=0;i<model_.count();++i){
    const SketchpadModel::Stroke &s=model_.at(i);
    const uint16_t ink=s.tool==SketchpadModel::Eraser?PAPER:
                       s.tool==SketchpadModel::Pencil?GRAPHITE:INK;
    d.drawLine(s.x0,s.y0,s.x1,s.y1,ink);
  }
  // A cursor frame distinct from the pen stroke; no overlay framebuffer.
  d.drawRect(cx_-3,cy_-3,7,7,CURSOR);
  const uint16_t text=INK;
  d.setTextColor(text,PAPER);d.setTextFont(1);d.setTextSize(1);
  d.setCursor(32,34);
  d.print(String("P")+String(selected_+1)+" "+TOOLS[tool_]+"  "+model_.count()+"/"+SketchpadModel::MAX_STROKES);
  d.setCursor(33,282);d.print("Dpad draw  OPT tool  B undo");
  if(message_.length()){
    d.setCursor(33,268);
    d.print(message_.substring(0,32));
  }
}
void SketchpadApp::draw(AppContext &ctx){
  ctx.ui.chrome("Sketchpad",WiFi.status()==WL_CONNECTED,false,
                ctx.storage.mounted(),ctx.settings.data().hour12);
  if(!editor_){
    for(int i=0;i<3;++i){
      const String path=String(ROOT)+"/page"+String(i+1)+".qsk";
      ctx.ui.listItem(i,"Note",String("Page ")+String(i+1),
                      !ctx.storage.mounted()?"microSD required":
                      ctx.storage.exists(path)?"Saved sketch":"Empty / import legacy",
                      i==selected_);
    }
    for(int i=3;i<SymbianUI::LIST_VISIBLE;++i)ctx.ui.clearListRow(i);
    ctx.ui.softkeys("Import","Open","Back");
    if(message_.length()){
      auto &d=ctx.ui.display();ThemeColors theme=ctx.ui.c();
      d.setTextFont(1);d.setTextSize(1);d.setTextColor(theme.text,theme.bg);
      d.setCursor(12,202);d.print(message_.substring(0,32));
    }
  }else{
    paintCanvas(ctx);
    ctx.ui.softkeys("Tool","Save","Back");
  }
  if(confirm_){
    static const char *const choices[]={"Save & close","Discard changes","Keep drawing"};
    ctx.ui.popupMenu(choices,3,confirmChoice_,0,3);
    ctx.ui.softkeys("","Select","Cancel");
  }
}
ScreenId SketchpadApp::handle(AppContext &ctx,const KeyEvent &e){
  if(!e.pressed || e.longPress)return ScreenId::Sketchpad;
  if(confirm_){
    if(e.key==Key::Up){confirmChoice_=(confirmChoice_+2)%3;draw(ctx);}
    else if(e.key==Key::Down){confirmChoice_=(confirmChoice_+1)%3;draw(ctx);}
    else if(e.key==Key::A || e.key==Key::B){confirm_=false;draw(ctx);}
    else if(e.key==Key::Start || e.key==Key::Select){
      const int action=confirmChoice_;confirm_=false;
      if(action==0 && !save(ctx)){draw(ctx);return ScreenId::Sketchpad;}
      if(action!=2){editor_=false;dirty_=false;message_=String();}
      draw(ctx);
    }
    return ScreenId::Sketchpad;
  }
  if(!editor_){
    if((e.key==Key::Up || e.key==Key::Down) && !e.repeat){
      selected_=(selected_+(e.key==Key::Up?2:1))%3;message_=String();draw(ctx);
    }else if(e.key==Key::Start || e.key==Key::Select){
      if(load(ctx)){editor_=true;cx_=115;cy_=139;tool_=SketchpadModel::Pen;}
      draw(ctx);
    }else if(e.key==Key::Option){importLegacy(ctx);draw(ctx);}
    else if(e.key==Key::A || e.key==Key::B)return ScreenId::Applications;
    return ScreenId::Sketchpad;
  }
  if(e.key==Key::Up||e.key==Key::Down||e.key==Key::Left||e.key==Key::Right){
    const int step=e.repeat?3:2;
    int nx=cx_,ny=cy_;
    if(e.key==Key::Up)ny-=step;
    if(e.key==Key::Down)ny+=step;
    if(e.key==Key::Left)nx-=step;
    if(e.key==Key::Right)nx+=step;
    if(nx<27)nx=27;if(nx>228)nx=228;
    if(ny<62)ny=62;if(ny>259)ny=259;
    if(nx!=cx_||ny!=cy_){
      if(model_.append(cx_,cy_,nx,ny,tool_)){cx_=nx;cy_=ny;dirty_=true;message_=String();}
      else message_="Page full: save or undo";
    }
    draw(ctx);
  }else if(e.key==Key::Option && !e.repeat){
    tool_=(tool_+1)%3;message_=String();draw(ctx);
  }else if(e.key==Key::B && !e.repeat){
    if(model_.undo()){dirty_=true;message_="Last stroke undone";}
    draw(ctx);
  }else if(e.key==Key::Start && !e.repeat){save(ctx);draw(ctx);}
  else if(e.key==Key::A){
    if(dirty_){confirm_=true;confirmChoice_=2;draw(ctx);}
    else{editor_=false;message_=String();draw(ctx);}
  }
  return ScreenId::Sketchpad;
}
