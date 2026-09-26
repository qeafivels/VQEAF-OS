#pragma once
#include <Arduino.h>
#include "StorageService.h"
#include "BrowserCookieJar.h"

struct BrowserLine {
  char text[58];
  int8_t link;
  BrowserLine() : text{0}, link(-1) {}
};

struct BrowserImage {
  char url[192];
  char alt[64];
  int16_t line;
  BrowserImage() : url{0}, alt{0}, line(-1) {}
};

struct BrowserLink {
  char url[192];
  char label[48];
  BrowserLink() : url{0}, label{0} {}
};

// In-OS adapter for the Qeafbrowser keypad-browser architecture. It keeps the
// same small-device principles: HTTP/HTTPS, redirects, fixed pools, no JS/CSS
// engine and no allocation in the input loop.
class BrowserService {
public:
  // Owns one fixed pool set; release it on host teardown. The firmware's
  // global BrowserService retains its pools for the entire OS lifetime.
  BrowserService() = default;
  ~BrowserService();
  BrowserService(const BrowserService &) = delete;
  BrowserService &operator=(const BrowserService &) = delete;
  static constexpr int MAX_LINES = 84;
  static constexpr int MAX_LINKS = 24;
  static constexpr int MAX_IMAGES = 8;
  static constexpr int HISTORY_MAX = 16;
  static constexpr int FORWARD_MAX = 12;
  static constexpr int BOOKMARK_MAX = 12;

  bool begin(StorageService *storage = nullptr);
  bool available() const { return poolsReady; }
#if defined(VQEAF_PERF_DIAG)
  void diagnosticPage(); // Isolated 72-line layout, no network or persistence.
#endif
  bool load(const String &inputUrl);
  bool reload();
  bool goBack();
  bool goForward();
  bool canGoForward() const { return forwardUsed > 0; }
  bool openLink(int index);
  bool bookmarkCurrent();
  int bookmarkCount() const { return bookmarkUsed; }
  const char *bookmarkAt(int i) const { return bookmarks && i >= 0 && i < bookmarkUsed ? bookmarks[i] : ""; }
  bool download(const String &inputUrl, String &savedPath, String &error);
  bool pageFromCache() const { return cachedPage; }

  const char *title() const { return pageTitle; }
  const char *url() const { return retryPending ? requestedUrl : currentUrl; }
  // Navigation failure keeps the requested URL so Reload retries the failing
  // address instead of unexpectedly opening the previous page.
  bool hasPendingRetry() const { return retryPending; }
  int status() const { return httpStatus; }
  const char *error() const { return errorText; }
  int lineCount() const { return lineUsed; }
  const BrowserLine &lineAt(int i) const { return lines[constrain(i, 0, max(0, lineUsed - 1))]; }
  int linkCount() const { return linkUsed; }
  int imageCount() const {return imageUsed;}
  const BrowserImage &imageAt(int i) const {return images[constrain(i,0,max(0,imageUsed-1))];}
  const BrowserLink &linkAt(int i) const { return links[constrain(i, 0, max(0, linkUsed - 1))]; }

  static bool normalizeUrl(const String &input, char *out, size_t cap);
  static bool resolveUrl(const char *base, const char *href, char *out, size_t cap);

private:
  // Large browser pools are allocated once from PSRAM in begin() so adding the
  // browser does not permanently consume ~12 KB of scarce internal DRAM/BSS.
  BrowserLine *lines = nullptr;
  BrowserLink *links = nullptr;
  BrowserImage *images = nullptr;
  int imageUsed = 0;
  char (*history)[192] = nullptr;
  char (*forward)[192] = nullptr;
  int forwardUsed = 0;
  char (*bookmarks)[192] = nullptr;
  BrowserCookieJar *cookieJar = nullptr;
  int bookmarkUsed = 0;
  bool poolsReady = false;
  StorageService *storage = nullptr;
  bool cachedPage = false;
  int historyUsed = 0;
  char currentUrl[192] = {0};
  char requestedUrl[192] = {0};
  bool retryPending = false;
  char pageTitle[64] = {0};
  char errorText[80] = {0};
  int httpStatus = 0;
  int lineUsed = 0;
  int linkUsed = 0;

  bool fetchAndParse(const char *url, bool pushHistory);
  bool renderInternal(const char *url, bool addHistory);
  void loadBookmarks();
  void loadCookies();
  bool saveCookies();
  bool saveBookmarks();
  void resetPage();
  void parseHtml(const char *src, size_t len);
  int addLink(const char *href, const char *label);
  void addWrappedText(const char *text, int linkIndex = -1);
  void pushHistory(const char *url);
  uint32_t cacheKey(const char *url) const;
  String cachePath(const char *url) const;
  bool loadCache(const char *url, char *body, size_t cap, size_t &used);
  void saveCache(const char *url, const char *body, size_t used);
  static String safeDownloadName(const char *url);
};
