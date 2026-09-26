// Portable cold-process recovery simulation for actual jar/thumbnail codecs.
// Firmware separately checks StorageService.writeAtomic and SD/LittleFS.
#include "../src/services/BrowserCookieJar.h"
#include "../src/services/BrowserThumbFormat.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
static constexpr const char *URL="https://qb-recovery.invalid/";
static constexpr const char *HTML="<html><title>QB RECOVERY FIXTURE 1</title></html>";
static const size_t SIZE=BrowserThumbFormat::WIDTH*BrowserThumbFormat::HEIGHT*2;
static void put(const std::string &filename,const void *p,size_t n){
  std::ofstream f(filename,std::ios::binary|std::ios::trunc);
  if(!f||!f.write((const char*)p,(std::streamsize)n))std::exit(2);
}
static std::vector<char> get(const std::string &filename){
  std::ifstream f(filename,std::ios::binary);
  if(!f)return {};
  return std::vector<char>((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
}
static bool stage(const std::string &dir){
  BrowserCookieJar cookies;
  if(!cookies.ingest(URL,"qb_probe=synthetic; Path=/; Secure"))return false;
  char blob[4096]={};
  size_t used=cookies.serialize(blob,sizeof(blob));
  if(!used)return false;
  put(dir+"/html",(const void*)HTML,std::strlen(HTML));
  put(dir+"/cookie",blob,used);
  std::vector<uint8_t> pixels(SIZE);
  for(size_t i=0;i<SIZE;++i)pixels[i]=(uint8_t)((i*37U+11U)&0xffU);
  const auto header=BrowserThumbFormat::make(URL,pixels.data(),pixels.size());
  std::vector<char> tile(sizeof(header)+SIZE);
  std::memcpy(tile.data(),&header,sizeof(header));
  std::memcpy(tile.data()+sizeof(header),pixels.data(),SIZE);
  put(dir+"/thumb",tile.data(),tile.size());
  // Commit is deliberately last: an interruption before this cannot PASS.
  const char marker='F';put(dir+"/marker",&marker,1);
  return true;
}
static bool verify(const std::string &dir){
  const auto marker=get(dir+"/marker");
  if(marker.size()!=1||marker[0]!='F')return false;
  const auto html=get(dir+"/html");
  if(html.size()!=std::strlen(HTML)||
     std::memcmp(html.data(),HTML,html.size()))return false;
  auto record=get(dir+"/cookie");
  if(record.empty()||record.size()>4095)return false;
  record.push_back(0);
  BrowserCookieJar jar;
  if(!jar.deserialize(record.data()))return false;
  char header[128]={};
  if(!jar.requestHeader(URL,header,sizeof(header))||
     std::strcmp(header,"qb_probe=synthetic"))return false;
  if(jar.requestHeader("https://other.invalid/",header,sizeof(header))||
     jar.requestHeader("http://qb-recovery.invalid/",header,sizeof(header)))return false;
  const auto tile=get(dir+"/thumb");
  if(tile.size()!=sizeof(BrowserThumbFormat::Header)+SIZE)return false;
  BrowserThumbFormat::Header h{};
  std::memcpy(&h,tile.data(),sizeof(h));
  if(!BrowserThumbFormat::valid(h,URL,(const uint8_t*)tile.data()+sizeof(h),SIZE))
    return false;
  for(size_t i=0;i<SIZE;++i)
    if((uint8_t)tile[sizeof(h)+i]!=(uint8_t)((i*37U+11U)&0xffU))return false;
  return true;
}
int main(int argc,char **argv){
  if(argc!=3)return 2;
  const std::string mode=argv[1],dir=argv[2];
  const bool ok=mode=="stage"?stage(dir):mode=="verify"?verify(dir):false;
  std::printf("[QB][HOST_RECOVERY] mode=%s result=%s\n",mode.c_str(),ok?"PASS":"FAIL");
  return ok?0:1;
}
