#include "BrowserThumbnailCache.h"
#include "BrowserThumbFormat.h"
#include "BrowserService.h"
#include "StorageService.h"
#include "TrustedTls.h"
#include "HttpChunkedDecoder.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <TJpg_Decoder.h>
#include <PNGdec.h>
#include <esp_heap_caps.h>
#include <new>
#include <string.h>

namespace {
constexpr size_t PIXELS=BrowserThumbnailCache::W*BrowserThumbnailCache::H;
constexpr size_t IMAGE_LIMIT=96*1024;
using BrowserThumbFormat::MAGIC;
using DiskHeader=BrowserThumbFormat::Header;
StorageService *fallbackStorage=nullptr;
static char diskName[64];
static bool persistentIsLittleFS=false;
static fs::FS *persistentFs() {
  if(persistentIsLittleFS)return &LittleFS;
  if(fallbackStorage && fallbackStorage->mounted())return &fallbackStorage->fs();
  return nullptr;
}
static String pathFor(uint64_t key) {
  snprintf(diskName,sizeof(diskName),"/%s/T%08lX%08lX.bin",
           persistentIsLittleFS?"qbthumb":"System/Cache/Thumbs",
           (unsigned long)(key>>32),(unsigned long)(key&0xffffffffUL));
  return String(diskName);
}
struct DecodeContext {
  uint16_t *dst=nullptr;
  int srcW=0,srcH=0;
  int blocks=0;
  PNG *png=nullptr;
};
DecodeContext decoding;
static uint16_t pngLine[1024];

bool jpegTile(int16_t x,int16_t y,uint16_t w,uint16_t h,uint16_t *pixels) {
  if(!decoding.dst||!pixels||decoding.srcW<=0||decoding.srcH<=0)return false;
  if((++decoding.blocks&15)==0)yield();
  // Pull exactly one nearest-neighbour source texel per destination pixel.
  // MCU callbacks can arrive in any order; no 240x320 sprite is needed.
  for(int oy=0;oy<BrowserThumbnailCache::H;++oy) {
    int sy=oy*decoding.srcH/BrowserThumbnailCache::H;
    if(sy<y||sy>=y+h)continue;
    for(int ox=0;ox<BrowserThumbnailCache::W;++ox) {
      int sx=ox*decoding.srcW/BrowserThumbnailCache::W;
      if(sx<x||sx>=x+w)continue;
      uint16_t raw=pixels[(sy-y)*w+(sx-x)];
      // JPEG decoder MCU RGB565 is in network order when drawn by TFT_eSPI
      // with swapBytes=true; store CPU-native RGB565 for pushImage(false).
      decoding.dst[oy*BrowserThumbnailCache::W+ox]=(raw>>8)|(raw<<8);
    }
  }
  return true;
}
int pngTile(PNGDRAW *draw) {
  if(!decoding.dst||!decoding.png||!draw||decoding.srcW>1024)return 1;
  decoding.png->getLineAsRGB565(draw,pngLine,PNG_RGB565_LITTLE_ENDIAN,0xFFFFFFFF);
  for(int oy=0;oy<BrowserThumbnailCache::H;++oy) {
    if(oy*decoding.srcH/BrowserThumbnailCache::H!=draw->y)continue;
    for(int ox=0;ox<BrowserThumbnailCache::W;++ox)
      decoding.dst[oy*BrowserThumbnailCache::W+ox]=
        pngLine[ox*decoding.srcW/BrowserThumbnailCache::W];
  }
  if((++decoding.blocks&15)==0)yield();
  return 1;
}
bool decodeImage(const uint8_t *buf,size_t size,uint16_t *output) {
  if(!buf||!size||!output)return false;
  for(size_t i=0;i<PIXELS;++i)output[i]=0xC618;
  decoding=DecodeContext();
  decoding.dst=output;
  if(size>=3&&buf[0]==0xFF&&buf[1]==0xD8&&buf[2]==0xFF) {
    uint16_t sw=0,sh=0;
    if(!TJpgDec.getJpgSize(&sw,&sh,buf,(uint32_t)size)||
       sw==0||sh==0||sw>4096||sh>4096) return false;
    uint8_t scale=1;
    while(scale<8&&(sw/scale>160||sh/scale>160))scale<<=1;
    decoding.srcW=(sw+scale-1)/scale;
    decoding.srcH=(sh+scale-1)/scale;
    TJpgDec.setCallback(jpegTile);
    TJpgDec.setJpgScale(scale);
    TJpgDec.drawJpg(0,0,buf,(uint32_t)size);
    bool ok=decoding.blocks>0;
    decoding.dst=nullptr;return ok;
  }
  const uint8_t signature[]={137,80,78,71,13,10,26,10};
  if(size<8||memcmp(buf,signature,8))return false;
  void *memory=heap_caps_malloc(sizeof(PNG),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  if(!memory)return false;
  PNG *png=new(memory) PNG;
  PNG_DRAW_CALLBACK *cb=pngTile;
  int rc=png->openRAM((uint8_t*)buf,(int)size,cb);
  if(rc==PNG_SUCCESS) {
    decoding.srcW=png->getWidth();decoding.srcH=png->getHeight();
    if(decoding.srcW>0&&decoding.srcW<=1024 &&
       decoding.srcH>0&&decoding.srcH<=4096) {
      decoding.png=png;rc=png->decode(nullptr,0);
    } else rc=-1;
    png->close();
  }
  bool ok=rc==PNG_SUCCESS&&decoding.blocks>0;
  decoding.png=nullptr;decoding.dst=nullptr;
  png->~PNG();free(png);return ok;
}
} // namespace

uint64_t BrowserThumbnailCache::hashUrl(const char *url) {
  return BrowserThumbFormat::fingerprint(url);
}
uint32_t BrowserThumbnailCache::crc32(const uint8_t *data,size_t len) {
  return BrowserThumbFormat::crc32(data,len);
}
bool BrowserThumbnailCache::begin(StorageService *storage) {
  fallbackStorage=storage;
  if(initialized)return true;
  for(auto &slot:slots) {
    slot.pixels=(uint16_t*)heap_caps_malloc(PIXELS*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!slot.pixels){
      for(auto &undo:slots){if(undo.pixels)free(undo.pixels);undo.pixels=nullptr;}
      return false;
    }
  }
  // Never automatically format an OS data partition to create a cache.
  persistentIsLittleFS=LittleFS.begin(false);
  if(persistentIsLittleFS)LittleFS.mkdir("/qbthumb");
  initialized=true;return true;
}
BrowserThumbnailCache::~BrowserThumbnailCache() {
  for(auto &slot:slots){if(slot.pixels)free(slot.pixels);slot.pixels=nullptr;}
}
BrowserThumbnailCache::Tile *BrowserThumbnailCache::find(uint64_t key) {
  for(auto &tile:slots)if(tile.valid&&tile.key==key){
    tile.touched=++accessCounter;return &tile;
  }
  return nullptr;
}
BrowserThumbnailCache::Tile *BrowserThumbnailCache::victim() {
  Tile *v=&slots[0];
  for(auto &tile:slots){
    if(!tile.valid)return &tile;
    if(tile.touched<v->touched)v=&tile;
  }
  v->valid=false;return v;
}
bool BrowserThumbnailCache::loadFlash(uint64_t key,Tile &tile) {
  fs::FS *fs=persistentFs();if(!fs||!tile.pixels)return false;
  File f=fs->open(pathFor(key),FILE_READ);
  if(!f||f.isDirectory()){if(f)f.close();return false;}
  DiskHeader head={};
  const bool sizeOkay=f.size()==sizeof(head)+PIXELS*2;
  const bool headerOkay=sizeOkay&&f.read((uint8_t*)&head,sizeof(head))==sizeof(head)
      &&head.magic==MAGIC&&head.key==key
      &&head.w==W&&head.h==H;
  const bool payloadOkay=headerOkay&&f.read((uint8_t*)tile.pixels,PIXELS*2)==PIXELS*2;
  f.close();
  if(!payloadOkay||crc32((const uint8_t*)tile.pixels,PIXELS*2)!=head.crc)return false;
  tile.key=key;tile.valid=true;tile.touched=++accessCounter;
  ++hitsFlash;return true;
}
void BrowserThumbnailCache::saveFlash(uint64_t key,const Tile &tile) {
  fs::FS *fs=persistentFs();if(!fs||!tile.pixels||!tile.valid)return;
  if(persistentIsLittleFS && LittleFS.totalBytes()-LittleFS.usedBytes()<65536UL)return;
  if(!persistentIsLittleFS&&fallbackStorage)
    fallbackStorage->ensureDir(StoragePaths::CACHE_THUMBS);
  const String target=pathFor(key),temp=target+".new";
  fs->remove(temp);
  File f=fs->open(temp,FILE_WRITE);if(!f)return;
  DiskHeader head={MAGIC,key,crc32((const uint8_t*)tile.pixels,PIXELS*2),W,H};
  bool ok=f.write((const uint8_t*)&head,sizeof(head))==sizeof(head)
       &&f.write((const uint8_t*)tile.pixels,PIXELS*2)==PIXELS*2;
  f.flush();f.close();
  if(!ok){fs->remove(temp);return;}
  fs->remove(target);
  if(!fs->rename(temp,target)){fs->remove(temp);return;}
  if(persistentIsLittleFS) {
    // Prune ONLY this browser's dedicated folder, never other OS resources.
    // File order here is approximate rather than persistent LRU.
    File folder=LittleFS.open("/qbthumb",FILE_READ);
    int count=0;
    if(folder&&folder.isDirectory()){
      File candidate=folder.openNextFile();
      while(candidate){++count;candidate.close();candidate=folder.openNextFile();}
      folder.close();
    }
    while(count>24){
      folder=LittleFS.open("/qbthumb",FILE_READ);
      if(!folder||!folder.isDirectory())break;
      bool removed=false;
      File candidate=folder.openNextFile();
      while(candidate){
        String old=candidate.name();
        candidate.close();
        if(old!=target&&LittleFS.remove(old)){removed=true;--count;break;}
        candidate=folder.openNextFile();
      }
      folder.close();
      if(!removed)break;
    }
  } else if(fallbackStorage) {
    fallbackStorage->pruneFlatDirectory(StoragePaths::CACHE_THUMBS,24,256UL*1024UL);
  }
}
bool BrowserThumbnailCache::has(const char *url) {
  if(!initialized||!url||!url[0])return false;
  const uint64_t key=hashUrl(url);
  if(find(key)){++hitsRam;return true;}
  // Check existence before evicting a valid PSRAM tile; a missing image
  // must never flush another site's cached preview on every redraw.
  fs::FS *fs=persistentFs();
  if(!fs||!fs->exists(pathFor(key)))return false;
  Tile *slot=victim();
  return loadFlash(key,*slot);
}
bool BrowserThumbnailCache::draw(TFT_eSPI &tft,const char *url,int x,int y) {
  if(!has(url))return false;
  Tile *tile=find(hashUrl(url));
  if(!tile)return false;
  tft.pushImage(x,y,W,H,tile->pixels);return true;
}
bool BrowserThumbnailCache::fetchAndDecode(const char *url,uint16_t *pixels) {
  if(!url||!pixels||strncmp(url,"https://",8)||WiFi.status()!=WL_CONNECTED)return false;
  uint8_t *body=(uint8_t*)heap_caps_malloc(IMAGE_LIMIT,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  if(!body)return false;
  char current[192],redirected[192];snprintf(current,sizeof(current),"%s",url);
  bool success=false;
  for(int hop=0;hop<4;++hop) {
    HTTPClient http;http.setConnectTimeout(5000);http.setTimeout(6000);
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    http.setUserAgent("Opera/9.80 (J2ME/MIDP; Opera Mini/4.5) Qeafbrowser-VQEAF/2.2");
    const char *headers[]={"Content-Type","Transfer-Encoding"};
    http.collectHeaders(headers,2);
    WiFiClientSecure secure;String err;
    if(!TrustedTls::configure(secure,err)||!http.begin(secure,current))break;
    http.addHeader("Accept","image/jpeg,image/png");
    http.addHeader("Accept-Encoding","identity");
    int code=http.GET();
    if(code>=300&&code<400){
      String location=http.getLocation();
      bool ok=location.length()&&BrowserService::resolveUrl(current,location.c_str(),
                                                            redirected,sizeof(redirected))
                  &&!strncmp(redirected,"https://",8);
      http.end();if(!ok)break;
      snprintf(current,sizeof(current),"%s",redirected);continue;
    }
    if(code<200||code>=300){http.end();break;}
    String mime=http.header("Content-Type");mime.toLowerCase();
    if(!mime.startsWith("image/jpeg")&&!mime.startsWith("image/png")){http.end();break;}
    const int expected=http.getSize();
    String transfer=http.header("Transfer-Encoding");
    transfer.toLowerCase();
    const bool chunked=transfer.indexOf("chunked")>=0;
    if(expected>(int)IMAGE_LIMIT || (expected<=0&&!chunked)){http.end();break;}
    WiFiClient *stream=http.getStreamPtr();
    size_t read=0;
    const uint32_t started=millis();
    uint32_t last=started;
    auto next=[&]()->int {
      while(true) {
        if((uint32_t)(millis()-started)>18000UL||
           (uint32_t)(millis()-last)>4000UL)return -1;
        if(stream->available()){
          int value=stream->read();if(value>=0){last=millis();return value;}
        }
        if(!http.connected())return -1;
        delay(1);
      }
    };
    bool ok=true;
    if(chunked) {
      uint32_t total=0;const char *why="";
      auto sink=[&](const uint8_t *p,size_t len)->bool {
        if(read+len>IMAGE_LIMIT)return false;
        memcpy(body+read,p,len);read+=len;return true;
      };
      ok=HttpChunkedDecoder::decodeTo(next,sink,IMAGE_LIMIT,total,why)&&total==read;
    }else{
      while(read<(size_t)expected){
        int value=next();if(value<0){ok=false;break;}
        body[read++]=(uint8_t)value;
      }
    }
    http.end();
    if(ok&&read>8)success=decodeImage(body,read,pixels);
    break;
  }
  free(body);return success;
}
bool BrowserThumbnailCache::prefetch(const char *url) {
  if(!initialized||!url||strncmp(url,"https://",8))return false;
  if(has(url))return true;
  Tile *tile=victim();
  if(!fetchAndDecode(url,tile->pixels)){++failures;return false;}
  tile->key=hashUrl(url);tile->valid=true;tile->touched=++accessCounter;
  saveFlash(tile->key,*tile);return true;
}
