// Host simulation of the actual firmware Lua runtime. Input MUST be a
// cryptographically verified source extracted from a signed QEAPP/2 package.
#include "QeLuaRuntime.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <stdint.h>
static unsigned rects=0,texts=0;
static uint32_t tick(void*) {
 using namespace std::chrono;
 return uint32_t(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}
static void rect(void*,int x,int y,int w,int h,uint16_t) {
 if(x<0||y<0||w<0||h<0||x+w>240||y+h>270)std::abort();
 ++rects;
}
static void text(void*,int,int,const char*,uint16_t){++texts;}
int main(int argc,char**argv){
 if(argc!=2){std::fputs("usage: real_package_runtime <verified.lua>\n",stderr);return 2;}
 std::ifstream input(argv[1],std::ios::binary);
 std::string script((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());
 if(!input || script.empty()){std::fputs("No verified Lua source\n",stderr);return 3;}
 QeLuaRuntime vm;QeLuaRuntime::Draw draw{rect,text,tick,nullptr};
 if(!vm.start(script.data(),script.size(),draw,192*1024)){
  std::fprintf(stderr,"FAIL start: %.180s\n",vm.error());return 4;
 }
 for(int f=0;f<60;++f){
  if(!vm.update(.05f) || !vm.render()){
   std::fprintf(stderr,"FAIL frame=%d error=%.180s\n",f,vm.error());return 5;
  }
  if(f==2||f==3) {
   if(!vm.key("right",f==2)){std::fprintf(stderr,"FAIL right key: %.180s\n",vm.error());return 6;}
  }
  if(f==8||f==9) {
   if(!vm.key("start",f==8)){std::fprintf(stderr,"FAIL start key: %.180s\n",vm.error());return 7;}
  }
 }
 std::printf("PASS VERIFIED_SIGNED_LUA_HOST frames=60 rects=%u texts=%u peak_heap=%lu\n",
    rects,texts,(unsigned long)vm.peakHeapUsed());
 return 0;
}
