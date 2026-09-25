#include "apps/SketchpadModel.h"
#include <cassert>
#include <cstdio>
#include <cstring>

int main(){
  SketchpadModel p;
  uint8_t bytes[SketchpadModel::MAX_BYTES]={0};
  assert(!p.append(0,55,24,90,0));
  assert(!p.append(24,55,240,90,0));
  assert(!p.append(24,40,55,55,0));
  assert(!p.append(24,55,55,281,0));
  assert(!p.append(24,55,55,90,3));
  assert(p.append(24,65,50,70,SketchpadModel::Pen));
  assert(p.append(50,70,70,70,SketchpadModel::Pencil));
  assert(p.undo()&&p.count()==1);
  size_t n=p.encode(bytes,sizeof bytes);
  assert(n==SketchpadModel::HEADER_BYTES+SketchpadModel::STROKE_BYTES);
  SketchpadModel loaded;
  assert(loaded.decode(bytes,n)&&loaded.count()==1);
  assert(loaded.at(0).x0==24&&loaded.at(0).y0==65);
  assert(!loaded.decode(bytes,n-1)&&loaded.count()==1);
  bytes[n-1]^=1;
  assert(!loaded.decode(bytes,n)&&loaded.count()==1); // SHA-like integrity check
  bytes[n-1]^=1;
  bytes[6]=255;
  assert(!loaded.decode(bytes,n)&&loaded.count()==1);
  n=p.encode(bytes,sizeof bytes);
  assert(loaded.decode(bytes,n));
  static const char legacy[]="31 40 238 299 4660\r\n31 49 34 50 31727\n";
  assert(loaded.importLegacy(legacy,sizeof legacy-1));
  assert(loaded.count()==2);
  assert(loaded.at(0).y0==44&&loaded.at(0).y1==277); // historical UI clipped
  assert(loaded.at(0).x1==228&&loaded.at(1).tool==SketchpadModel::Pencil);
  static const char bad[]="21 55 50 65 42\n../payload\n";
  assert(!loaded.importLegacy(bad,sizeof bad-1)&&loaded.count()==2);
  static const char extra[]="21 55 50 65 42 extra\n";
  assert(!loaded.importLegacy(extra,sizeof extra-1)&&loaded.count()==2);
  static const char overflow[]="999999999 55 50 65 42\n";
  assert(!loaded.importLegacy(overflow,sizeof overflow-1)&&loaded.count()==2);
  SketchpadModel cap;
  for(int i=0;i<SketchpadModel::MAX_STROKES;++i)
    assert(cap.append(30,80,30+i%100,82+i%100,SketchpadModel::Pen));
  assert(!cap.append(30,80,40,90,SketchpadModel::Pen));
  n=cap.encode(bytes,sizeof bytes);
  assert(n==sizeof bytes);
  SketchpadModel roundtrip;
  assert(roundtrip.decode(bytes,n)&&roundtrip.count()==SketchpadModel::MAX_STROKES);
  for(int i=0;i<SketchpadModel::MAX_STROKES;++i)
    assert(roundtrip.at(i).x1==cap.at(i).x1);
  assert(roundtrip.undo()&&roundtrip.count()==SketchpadModel::MAX_STROKES-1);
  puts("PASS: bounded native sketches, canonical save/restore, CRC-like FNV, strict legacy importer, bounds and undo");
}
