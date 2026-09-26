#include "../src/services/BrowserMotion.h"
#include "../src/services/BrowserCookieJar.h"
#include <cassert>
#include <cstring>
#include <cstdio>

static void motionTest(){
  BrowserMotion m;m.reset(400);
  assert(m.documentPixels()==6400&&m.maxScroll()==6192);
  for(int i=0;i<40;++i)m.scrollPixels(16);
  assert(m.targetPixel()>0);
  m.tick(100);
  for(uint32_t t=117;t<2500;t+=17)m.tick(t);
  assert(m.pixel()>=0&&m.pixel()<=m.maxScroll());
  assert(m.pixel()==m.targetPixel());
  m.toggleOverview();assert(m.overview());
  for(int z=1;z<20;++z)m.zoom(1);
  assert(m.zoomValue()==8);
  for(int z=1;z<20;++z)m.zoom(-1);
  assert(m.zoomValue()==1);
  m.overviewPan(1);assert(m.targetPixel()<=m.maxScroll());
  m.toggleOverview();assert(!m.overview());
  m.jumpToLine(100000);assert(m.targetPixel()==m.maxScroll());
  m.jumpToLine(-10);assert(m.targetPixel()==0);
  m.setDocumentLines(2);assert(m.maxScroll()==0);
  puts("PASS BrowserMotion: overview, 1..8 zoom, pixel clamp, inertia");
}
static void cookieTest(){
  BrowserCookieJar jar;
  assert(jar.ingest("https://example.com/login","sid=abc123; Path=/; Secure; HttpOnly"));
  char h[512];
  assert(jar.requestHeader("https://example.com/account",h,sizeof(h)));
  assert(std::strstr(h,"sid=abc123"));
  assert(!jar.requestHeader("http://example.com/account",h,sizeof(h)));
  assert(!jar.requestHeader("https://evil.example.com/account",h,sizeof(h)));
  assert(!jar.ingest("https://example.com/","evil=x; Domain=other.com"));
  assert(!jar.ingest("http://example.com/","__Secure-id=y; Secure"));
  assert(!jar.ingest("https://example.com/","__Host-id=x; Domain=example.com; Path=/; Secure"));
  assert(jar.ingest("https://example.com/","pref=on; Path=/settings"));
  assert(!jar.requestHeader("https://example.com/another",h,sizeof(h)) ||
         !std::strstr(h,"pref=on"));
  char serialized[4096];
  size_t len=jar.serialize(serialized,sizeof(serialized));assert(len>3);
  BrowserCookieJar loaded;
  assert(loaded.deserialize(serialized));
  assert(loaded.requestHeader("https://example.com/settings",h,sizeof(h)));
  assert(std::strstr(h,"sid=abc123")&&std::strstr(h,"pref=on"));
  assert(loaded.ingest("https://example.com/","sid=; Path=/; Max-Age=0"));
  assert(!loaded.requestHeader("https://example.com/account",h,sizeof(h)));
  // Reject malformed on-disk input instead of importing cross-origin secrets.
  char broken[]="C1\nS\tevil.com\t/\tsid\tabc";assert(!loaded.deserialize(broken));
  puts("PASS BrowserCookieJar: persistence, host/path scoping, secure, deletion");
}
int main(){motionTest();cookieTest();return 0;}
