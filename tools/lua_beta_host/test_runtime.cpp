#include "QeLuaRuntime.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <chrono>
#include <stdint.h>
static int drawCount=0;
static uint32_t tick(void*) {
 using namespace std::chrono;
 return uint32_t(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}
static void rect(void*,int x,int y,int w,int h,uint16_t) {
 if(x<0||y<0||x+w>240||y+h>270)std::abort();
 ++drawCount;
}
static void text(void*,int,int,const char*,uint16_t) {++drawCount;}
static bool check(bool ok,const char *msg) {if(!ok)std::fprintf(stderr,"FAIL: %s\n",msg);return ok;}
int main(){
 QeLuaRuntime vm;
 const QeLuaRuntime::Draw cb{rect,text,tick,nullptr};
 const char plain[]="local p=0\nfunction on_key(key,down) if down and key=='right' then p=p+1 end end\nfunction on_update(dt) end\nfunction on_draw() engine.clear(0) engine.rect(p,2,3,4,65535) end\n";
 if(!check(vm.start(plain,sizeof plain-1,cb),"start"))return 1;
 if(!check(vm.key("right",true) && vm.update(.05f) && vm.render(),"callbacks"))return 2;
 if(!check(drawCount>=2,"drawing"))return 3;
 if(!check(vm.heapUsed()>0 && vm.peakHeapUsed()>0,"bounded heap telemetry"))return 4;
 if(!check(!vm.key("menu",true) && !vm.running(),"OS reserved key"))return 5;
 const char escape[]="function on_draw() return os.execute('echo no') end";
 if(!check(vm.start(escape,sizeof escape-1,cb),"sandbox startup"))return 6;
 if(!check(!vm.render() && !vm.running(),"no OS API"))return 7;
 const char loop[]="function on_update(dt) while true do end end";
 if(!check(vm.start(loop,sizeof loop-1,cb),"loop startup"))return 8;
 if(!check(!vm.update(.05f) && !vm.running(),"instruction hard limit"))return 9;
 if(!check(strstr(vm.error(),"budget")!=nullptr,"budget reason retained"))return 10;
 const char color[]="function on_draw() engine.rect(0,0,1,1,999999) end";
 if(!check(vm.start(color,sizeof color-1,cb),"bad color startup"))return 11;
 if(!check(!vm.render() && !vm.running(),"color bounds"))return 12;
 const char sprite[]="function on_draw() engine.blit1(5,6,8,1,string.char(255),31) end";
 if(!check(vm.start(sprite,sizeof sprite-1,cb),"sprite startup"))return 13;
 int before=drawCount;
 if(!check(vm.render() && drawCount==before+1,"bounded bitmap draw"))return 14;
 std::puts("PASS Lua 5.4 runtime callbacks, constrained host drawing, memory, OS key guard, no os/io, watchdog and sprite");
}
