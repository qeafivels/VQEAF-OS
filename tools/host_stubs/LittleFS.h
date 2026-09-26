#pragma once
#include "FS.h"
class LittleFSClass : public fs::FS {
public:
  bool begin(bool formatOnFail=false){(void)formatOnFail;return false;}
  uint64_t totalBytes() const{return 0;}
  uint64_t usedBytes() const{return 0;}
};
extern LittleFSClass LittleFS;
