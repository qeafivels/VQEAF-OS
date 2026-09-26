#pragma once
// Independent, bounded sketch state inspired by the legacy E524546-OS Paint
// UX; clean-room C++ implementation, NO legacy Lua/vendor runtime copied.
// Pure C++11; host-testable without Arduino, FS or framebuffer dependencies.
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

class SketchpadModel {
public:
  enum { MAX_STROKES=160, HEADER_BYTES=11, STROKE_BYTES=7,
         MAX_BYTES=HEADER_BYTES+MAX_STROKES*STROKE_BYTES };
  enum Tool : uint8_t { Pen=0, Pencil=1, Eraser=2 };
  struct Stroke {
    uint8_t x0;
    uint16_t y0;
    uint8_t x1;
    uint16_t y1;
    uint8_t tool;
  };
  static bool validPoint(int x,int y) {
    return x>=12 && x<=228 && y>=44 && y<=277;
  }
  SketchpadModel() : length_(0) {}
  int count() const { return length_; }
  const Stroke &at(int i) const { return strokes_[i]; }
  void clear() { length_=0; }
  bool append(int x0,int y0,int x1,int y1,uint8_t tool) {
    if (length_>=MAX_STROKES || !validPoint(x0,y0) || !validPoint(x1,y1) ||
        tool>Eraser) return false;
    Stroke &s=strokes_[length_++];
    s.x0=(uint8_t)x0;s.y0=(uint16_t)y0;s.x1=(uint8_t)x1;
    s.y1=(uint16_t)y1;s.tool=tool;
    return true;
  }
  bool undo() { if(!length_)return false;--length_;return true; }
  // File format: VQSK1 + LE16 count + LE32 FNV1a(payload) + 7B/stroke.
  // Exact size, checksum and point checks are required before replacing state.
  size_t encode(uint8_t *out,size_t cap) const {
    const size_t used=HEADER_BYTES+length_*STROKE_BYTES;
    if (!out || cap<used)return 0;
    memcpy(out,"VQSK1",5);
    out[5]=(uint8_t)length_;out[6]=(uint8_t)(length_>>8);
    for (int i=0;i<length_;++i) {
      const Stroke &s=strokes_[i];uint8_t *p=out+HEADER_BYTES+i*STROKE_BYTES;
      p[0]=s.x0;p[1]=(uint8_t)s.y0;p[2]=(uint8_t)(s.y0>>8);
      p[3]=s.x1;p[4]=(uint8_t)s.y1;p[5]=(uint8_t)(s.y1>>8);p[6]=s.tool;
    }
    const uint32_t hash=fnv(out+HEADER_BYTES,used-HEADER_BYTES);
    for(int j=0;j<4;++j)out[7+j]=(uint8_t)(hash>>(8*j));
    return used;
  }
  bool decode(const uint8_t *buf,size_t len) {
    if (!buf || len<HEADER_BYTES || memcmp(buf,"VQSK1",5))return false;
    const unsigned n=unsigned(buf[5])|(unsigned(buf[6])<<8);
    if(n>MAX_STROKES || len!=HEADER_BYTES+n*STROKE_BYTES)return false;
    uint32_t expected=0;
    for(int j=0;j<4;++j)expected|=uint32_t(buf[7+j])<<(8*j);
    if(fnv(buf+HEADER_BYTES,len-HEADER_BYTES)!=expected)return false;
    SketchpadModel next;
    for(unsigned i=0;i<n;++i){
      const uint8_t *p=buf+HEADER_BYTES+i*STROKE_BYTES;
      const int y0=int(p[1])+(int(p[2])<<8),y1=int(p[4])+(int(p[5])<<8);
      if(!next.append(p[0],y0,p[3],y1,p[6]))return false;
    }
    *this=next;
    return true;
  }
  // Explicit one-time import of legacy trangN.dat text copied onto SD.
  // Reject malformed or excessive input; never execute source or touch old data.
  bool importLegacy(const char *src,size_t bytes) {
    if(!src || bytes==0 || bytes>8192 || memchr(src,0,bytes))return false;
    SketchpadModel next;
    size_t pos=0;
    while(pos<bytes){
      size_t end=pos;
      while(end<bytes && src[end]!='\n' && src[end]!='\r')++end;
      if(end-pos>96)return false;
      if(end>pos){
        char line[98]={0};memcpy(line,src+pos,end-pos);
        long x0,y0,x1,y1,color;char extra=0;
        if(sscanf(line," %ld %ld %ld %ld %ld %c",&x0,&y0,&x1,&y1,&color,&extra)!=5 ||
           x0<0||x0>239||x1<0||x1>239||y0<0||y0>319||y1<0||y1>319||
           color<0||color>65535)return false;
        // Preserve useful legacy drawing even if its old canvas encroached on
        // the OS bar/footer; clamp to the modern content viewport.
        const int ax=x0<12?12:(x0>228?228:int(x0));
        const int bx=x1<12?12:(x1>228?228:int(x1));
        const int ay=y0<44?44:(y0>277?277:int(y0));
        const int by=y1<44?44:(y1>277?277:int(y1));
        // Existing notebook palettes are approximate; keep the old hue family.
        const uint8_t tool=color>=0x4000 && color<=0xAFFF?Pencil:Pen;
        if(!next.append(ax,ay,bx,by,tool))return false;
      }
      pos=end;
      while(pos<bytes && (src[pos]=='\n'||src[pos]=='\r'))++pos;
    }
    if(!next.count())return false;
    *this=next;
    return true;
  }
private:
  Stroke strokes_[MAX_STROKES];
  uint16_t length_;
  static uint32_t fnv(const uint8_t *p,size_t len){
    uint32_t v=2166136261UL;
    for(size_t i=0;i<len;++i){v^=p[i];v*=16777619UL;}
    return v;
  }
};
