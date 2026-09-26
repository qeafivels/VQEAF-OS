#pragma once
#include "StorageService.h"
#include "BrowserCookieJar.h"
#include "BrowserThumbFormat.h"
#include <LittleFS.h>
#include <esp_heap_caps.h>
#include <new>

// PERF_DIAG-only, isolated deterministic test records. Never reads/overwrites
// the user's browser cookies, cache, settings or downloaded files.
class BrowserRecoveryProbe {
  static constexpr const char *HTML="/System/Temp/qb_recovery_html.tmp";
  static constexpr const char *COOKIE="/System/Temp/qb_recovery_cookie.tmp";
  static constexpr const char *MARKER="/System/Temp/qb_recovery.marker";
  static constexpr const char *THUMB="/qbthumb/qb_recovery_probe.bin";
  static constexpr const char *TEST_URL="https://qb-recovery.invalid/";
  static constexpr size_t TILE_BYTES=BrowserThumbFormat::WIDTH*BrowserThumbFormat::HEIGHT*2;
  static bool readExact(fs::FS &fs,const char *path,uint8_t *buf,size_t n){
    File f=fs.open(path,FILE_READ);
    if(!f||f.isDirectory()){if(f)f.close();return false;}
    const bool ok=f.size()==n&&f.read(buf,n)==n;
    f.close();return ok;
  }
  static bool checkHtml(StorageService &storage){
    constexpr char PAYLOAD[]="<html><title>QB RECOVERY FIXTURE 1</title></html>";
    storage.recoverAtomicFile(HTML);
    uint8_t raw[sizeof(PAYLOAD)-1]={};
    return readExact(storage.fs(),HTML,raw,sizeof(raw)) &&
           memcmp(raw,PAYLOAD,sizeof(raw))==0;
  }
  static bool checkCookie(StorageService &storage) {
    storage.recoverAtomicFile(COOKIE);
    File f=storage.fs().open(COOKIE,FILE_READ);
    if(!f||f.isDirectory()){if(f)f.close();return false;}
    const size_t n=(size_t)f.size();
    if(!n||n>4095){f.close();return false;}
    char *record=(char*)heap_caps_malloc(4096,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    void *memory=heap_caps_malloc(sizeof(BrowserCookieJar),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!record||!memory){
      if(record)free(record);if(memory)free(memory);f.close();return false;
    }
    const bool read=f.readBytes(record,n)==n;
    record[n]=0;f.close();
    auto *jar=new(memory) BrowserCookieJar();
    char result[128]={};
    bool ok=read&&jar->deserialize(record)&&
       jar->requestHeader(TEST_URL,result,sizeof(result))&&
       strcmp(result,"qb_probe=synthetic")==0;
    // Domain and plaintext downgrade remain blocked after deserialize/reboot.
    char other[128]={};
    if(jar->requestHeader("https://other.invalid/",other,sizeof(other)))ok=false;
    if(jar->requestHeader("http://qb-recovery.invalid/",other,sizeof(other)))ok=false;
    jar->~BrowserCookieJar();free(memory);free(record);
    return ok;
  }
  static void fillTile(uint8_t *payload){
    for(size_t i=0;i<TILE_BYTES;++i)payload[i]=(uint8_t)((i*37U+11U)&0xffU);
  }
  static bool checkThumb(){
    if(!LittleFS.begin(false))return false; // Never format flash.
    uint8_t *tile=(uint8_t*)heap_caps_malloc(TILE_BYTES,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!tile)return false;
    File f=LittleFS.open(THUMB,FILE_READ);
    BrowserThumbFormat::Header h={};
    bool ok=f&&!f.isDirectory()&&
      f.size()==sizeof(h)+TILE_BYTES&&
      f.read((uint8_t*)&h,sizeof(h))==sizeof(h)&&
      f.read(tile,TILE_BYTES)==TILE_BYTES;
    if(f)f.close();
    if(ok){
      uint8_t *expected=(uint8_t*)heap_caps_malloc(TILE_BYTES,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
      if(!expected)ok=false;
      else {
        fillTile(expected);
        ok=BrowserThumbFormat::valid(h,TEST_URL,tile,TILE_BYTES)&&
           memcmp(tile,expected,TILE_BYTES)==0;
        free(expected);
      }
    }
    free(tile);return ok;
  }
public:
  static void stage(StorageService &storage) {
    if(!storage.mounted()||!storage.ensureDir(StoragePaths::TEMP)){
      Serial.println("[QB][RECOVERY] stage=INCONCLUSIVE reason=SD_NOT_READY");return;
    }
    constexpr char HTML_PAYLOAD[]="<html><title>QB RECOVERY FIXTURE 1</title></html>";
    bool html=storage.writeAtomic(HTML,(const uint8_t*)HTML_PAYLOAD,sizeof(HTML_PAYLOAD)-1);
    void *memory=heap_caps_malloc(sizeof(BrowserCookieJar),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    char *record=(char*)heap_caps_malloc(4096,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    bool cookies=false;
    if(memory&&record){
      auto *jar=new(memory)BrowserCookieJar();
      if(jar->ingest(TEST_URL,"qb_probe=synthetic; Path=/; Secure")){
        const size_t used=jar->serialize(record,4096);
        if(used)cookies=storage.writeAtomic(COOKIE,(const uint8_t*)record,used);
      }
      jar->~BrowserCookieJar();
    }
    if(memory)free(memory);if(record)free(record);
    bool flash=false;
    if(LittleFS.begin(false)){
      LittleFS.mkdir("/qbthumb");
      uint8_t *tile=(uint8_t*)heap_caps_malloc(TILE_BYTES,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
      if(tile){
        fillTile(tile);
        const auto hdr=BrowserThumbFormat::make(TEST_URL,tile,TILE_BYTES);
        const String pending=String(THUMB)+".new";
        LittleFS.remove(pending);
        File f=LittleFS.open(pending,FILE_WRITE);
        if(f){
          flash=f.write((const uint8_t*)&hdr,sizeof(hdr))==sizeof(hdr)&&
                 f.write(tile,TILE_BYTES)==TILE_BYTES;
          f.flush();f.close();
          if(flash){
            LittleFS.remove(THUMB);
            flash=LittleFS.rename(pending,THUMB);
          }
          if(!flash)LittleFS.remove(pending);
        }
        free(tile);
      }
    }
    // Commit marker LAST so an interrupted stage cannot fake a completed run.
    const char marker=flash?'F':'S';
    bool committed=html&&cookies&&
      storage.writeAtomic(MARKER,(const uint8_t*)&marker,1);
    Serial.printf("[QB][RECOVERY] stage=%s html=%s cookie=%s thumb=%s\n",
       committed?"PASS":"FAIL",html?"PASS":"FAIL",cookies?"PASS":"FAIL",
       flash?"FLASH_PASS":"FLASH_UNAVAILABLE");
  }
  static void verify(StorageService &storage){
    if(!storage.mounted()){
      Serial.println("[QB][RECOVERY] verify=INCONCLUSIVE reason=SD_NOT_READY");return;
    }
    storage.recoverAtomicFile(MARKER);
    uint8_t flag=0;
    if(!readExact(storage.fs(),MARKER,&flag,1)||(flag!='F'&&flag!='S')){
      Serial.println("[QB][RECOVERY] verify=INCONCLUSIVE reason=NO_COMMITTED_STAGE");
      return;
    }
    const bool html=checkHtml(storage);
    const bool cookie=checkCookie(storage);
    const bool thumb=flag=='F'?checkThumb():true;
    const bool pass=html&&cookie&&thumb;
    Serial.printf("[QB][RECOVERY] verify=%s html=%s cookie=%s thumb=%s boot_uptime_ms=%lu\n",
       pass?"PASS":"FAIL",html?"PASS":"FAIL",cookie?"PASS":"FAIL",
       flag=='F'?(thumb?"PASS":"FAIL"):"SKIPPED_FLASH_UNAVAILABLE",
       (unsigned long)millis());
  }
};