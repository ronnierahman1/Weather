#include "xda_feeds.h"
#include "rssFeedHandler.h"   // httpGET_Insecure(..), parseRssItems(..)

// Single source of truth for XDA sections.
const XdaFeedSpec kXdaFeeds[(int)XdaCat::COUNT] = {
  {"PC Hardware",          "https://www.xda-developers.com/feed/category/pc-hardware/"},
  {"CPU",                  "https://www.xda-developers.com/feed/processor/"},
  {"Storage",              "https://www.xda-developers.com/feed/storage/"},
  {"Monitors",             "https://www.xda-developers.com/feed/monitors/"},
  {"Input Devices",        "https://www.xda-developers.com/feed/input-devices/"},
  {"Productivity",         "https://www.xda-developers.com/feed/productivity/"},
  {"Software & Services",  "https://www.xda-developers.com/feed/software-and-services/"},
  {"Windows",              "https://www.xda-developers.com/feed/windows/"},
  {"Linux",                "https://www.xda-developers.com/feed/linux-hub/"},
  {"Devices",              "https://www.xda-developers.com/feed/devices/"},
  {"Gaming",               "https://www.xda-developers.com/feed/gaming/"},
  {"Single-Board Computers","https://www.xda-developers.com/feed/single-board-computers/"},
  {"Laptops",              "https://www.xda-developers.com/feed/laptops/"},
  {"Gaming Handhelds",     "https://www.xda-developers.com/feed/gaming-handhelds/"},
  {"Prebuilt PCs",         "https://www.xda-developers.com/feed/prebuilt-pc/"},
  {"Networking",           "https://www.xda-developers.com/feed/networking/"},
  {"Smart Home",           "https://www.xda-developers.com/feed/smart-home/"}
};

static int parseRssTitlesOnly(const String& xml, String* out, int cap) {
  // If your rssFeedHandler already has a titles-only helper, call it here.
  // Otherwise, reuse parseRssItems and copy only the title column.
//   const int MAX_TMP = 16;                         // small scratch (fits most feeds)
//   const int tmpCap = (cap < MAX_TMP) ? cap : MAX_TMP;
//   String tmp[tmpCap][2];
  return parseRssItems(xml, out, cap, "");    // reuse your existing robust parser
//   for (int i = 0; i < n; ++i) out[i] = tmp[i][0];
//   return n;
}

bool fetchXdaTitles(XdaCat cat, String* outTitles,String* outDates, String* outAuthors, int cap, int& outCount) {
  outCount = 0;
  const XdaFeedSpec& spec = kXdaFeeds[(int)cat];

  String body;
  if (!httpGET_Insecure(spec.url, body)) return false;

//   outCount = parseRssTitlesOnly(body, outTitles, cap);
  outCount = parseRssByTag(body, outTitles, cap, "title", "");
  parseRssByTag(body, outDates, cap, "pubDate", "");
  parseRssByTag(body, outAuthors, cap, "dc:creator", "");

  // Release large body buffer ASAP (saves heap):
  body = String(); 
  body.reserve(0);
  return outCount > 0;
}

bool fetchXdaTitles(XdaCat cat, String* outTitles, int cap, int& outCount) {
  outCount = 0;
  const XdaFeedSpec& spec = kXdaFeeds[(int)cat];

  String body;
  if (!httpGET_Insecure(spec.url, body)) return false;

//   outCount = parseRssTitlesOnly(body, outTitles, cap);
  outCount = parseRssTitlesOnly(body, outTitles, cap);

  // Release large body buffer ASAP (saves heap):
  body = String(); 
  body.reserve(0);
  return outCount > 0;
}

bool fetchXdaTitlesMany(const XdaCat* cats, int nCats,
                        String (*outTitles)[XDA_MAX_TITLES], int capEach,
                        int* outCounts)
{
  bool ok = true;
  for (int i = 0; i < nCats; ++i) {
    int cnt = 0;
    bool one = fetchXdaTitles(cats[i], outTitles[i], capEach, cnt);
    outCounts[i] = cnt;
    ok = ok && one;
    // (Optional) small delay to be nice to the site/CDN
    delay(50);
  }
  return ok;
}
