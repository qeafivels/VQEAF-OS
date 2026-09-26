#pragma once
#include <stdint.h>

// Pure planning logic: only invalidate expensive 3x3 previews when the page
// window or zoom changes. Focus/cursor and scrollbar have smaller dirty regions.
// Never caches pixel data, HTML, cookies, pointers or LCD framebuffers.
class BrowserOverviewDirty {
public:
  enum class Kind : uint8_t { Full, Selection, Progress, None };
  struct Plan {Kind kind; int previousSelected;};
  Plan update(bool force,int firstTile,int zoom,int selected,int progress){
    const Plan out = !valid || force || first!=firstTile || level!=zoom
       ? Plan{Kind::Full,selected}
       : selected!=focus ? Plan{Kind::Selection,focus}
       : progress!=bar ? Plan{Kind::Progress,focus}
       : Plan{Kind::None,focus};
    valid=true;first=firstTile;level=zoom;focus=selected;bar=progress;
    return out;
  }
  void invalidate(){valid=false;}
private:
  bool valid=false;
  int first=-1,level=0,focus=-1,bar=-1;
};
