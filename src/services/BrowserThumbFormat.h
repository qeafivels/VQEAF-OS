#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Portable persistent format shared by the ESP32 cache and host regression.
namespace BrowserThumbFormat {
static constexpr uint32_t MAGIC=0x32425451UL; // QTB2
static constexpr uint16_t WIDTH=64,HEIGHT=48;
struct __attribute__((packed)) Header {
  uint32_t magic;
  uint64_t key;
  uint32_t crc;
  uint16_t w,h;
};
inline uint64_t fingerprint(const char *url) {
  uint64_t hash=14695981039346656037ULL;
  if(url)for(const uint8_t *p=(const uint8_t*)url;*p;++p){
    hash^=*p;hash*=1099511628211ULL;
  }
  return hash;
}
inline uint32_t crc32(const uint8_t *data,size_t len) {
  uint32_t crc=UINT32_MAX;
  if(!data)return 0;
  for(size_t i=0;i<len;++i){
    crc^=data[i];
    for(int b=0;b<8;++b)crc=(crc>>1)^((crc&1U)?0xEDB88320U:0U);
  }
  return ~crc;
}
inline Header make(const char *url,const uint8_t *rgb,size_t length){
  Header h={MAGIC,fingerprint(url),crc32(rgb,length),WIDTH,HEIGHT};
  return h;
}
inline bool valid(const Header &h,const char *url,
                  const uint8_t *rgb,size_t length){
  return h.magic==MAGIC&&h.key==fingerprint(url)&&
         h.w==WIDTH&&h.h==HEIGHT&&
         length==size_t(WIDTH)*HEIGHT*2&&h.crc==crc32(rgb,length);
}
} // namespace BrowserThumbFormat
