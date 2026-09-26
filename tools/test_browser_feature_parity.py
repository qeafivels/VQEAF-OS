#!/usr/bin/env python3
"""Source/ABI regression gates; actual TLS/decoder/LCD requires ESP32-S3 run."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
app=(r/"src/apps/Apps.cpp").read_text()
api=(r/"src/apps/Apps.h").read_text()
motion=(r/"src/services/BrowserMotion.h").read_text()
cookies=(r/"src/services/BrowserCookieJar.h").read_text()
svc=(r/"src/services/BrowserService.cpp").read_text()
thumb=(r/"src/services/BrowserThumbnailCache.cpp").read_text()
fmt=(r/"src/services/BrowserThumbFormat.h").read_text()
main=(r/"src/main.cpp").read_text()
ci=(r/".github/workflows/qeafbrowser-native-ci.yml").read_text()
def gate(v,label):
    if not v:raise AssertionError(label)
    print("PASS",label)
gate("if(choice==13){motion.toggleOverview()" in app and
     "redrawOverview" in app and "tileIndex=firstTile+i" in app,
     "overview page-tile grid and keyboard route")
gate("motion.zoom(e.key==Key::Right?1:-1)" in app and
     "if(v<1) v=1;" in motion and "if(v>8) v=8;" in motion,
     "overview x1-x8 zoom bound")
gate("motion.scrollPixels(16)" in app and "motion.release()" in app and
     "now-lastMotionPaint)>=33UL" in app and
     "velocityQ8 = velocityQ8 * 230 / 256" in motion and
     "browserApp.tick(appCtx" in main,
     "fixed-point pixel scroll/inertia OS tick")
gate("imageUsed<MAX_IMAGES" in svc and
     "resolveUrl(currentUrl,source" in svc and
     "alt[0]?alt:\"[Image]\"" in svc,
     "image src extraction and accessible fallback")
gate("TJpgDec.getJpgSize" in thumb and "png->openRAM" in thumb and
     "PNG_RGB565_LITTLE_ENDIAN" in thumb and "jpegTile" in thumb,
     "JPEG/PNG RGB565 thumbnail decode paths")
gate("MALLOC_CAP_SPIRAM" in thumb and "LittleFS.begin(false)" in thumb and
     "fallbackStorage->pruneFlatDirectory" in thumb and
     "BrowserThumbFormat::crc32" in thumb and
     "head.key==key" in thumb,
     "three-slot PSRAM/LittleFS-or-SD CRC two-tier cache")
gate("TrustedTls::configure" in thumb and
     "strncmp(url,\"https://\",8)" in thumb and
     "if(!strncmp(redirected,\"https://\",8))" not in thumb and
     "&&!strncmp(redirected,\"https://\",8)" in thumb and
     "setInsecure" not in thumb,
     "verified thumbnail HTTPS with downgrade protection")
gate("const char *wantedHeaders[]" in svc and
     "http.addHeader(\"Cookie\",requestCookies)" in svc and
     "saveCookies()" in svc and "loadCookies()" in svc and
     "writeAtomic(kBrowserCookiesPath" in svc,
     "bounded cookie persistence and request integration")
gate("!strcmp(items[i].host,host)" in cookies and
     "if(secure&&!https)return false;" in cookies and
     "if(n!=strlen(host)||strncasecmpPortable(d,host,n))return false;" in cookies,
     "cookie exact-host secure boundary")
gate("tools/browser_parity_host.cpp" in ci and
     "test_browser_feature_parity.py" in ci,
     "CI runs feature-parity host regression")
print("PASS browser feature structural regression")
