#pragma once
#include <Arduino.h>
#include "xda_feeds.h"   // XdaCat, XDA_MAX_TITLES, kXdaFeeds

class EPaper;            // forward decl (provided by your display driver)

// Unify with feed layer capacity

static constexpr int numOfCategorys    = (int)XdaCat::COUNT;

struct xdaNewsItem {
  String title[XDA_MAX_TITLES];
  String Author[XDA_MAX_TITLES];
  String Date[XDA_MAX_TITLES];
  String link;
  String headerText;
};

// Back-compat: fetch a single category’s titles (defaults to PC Hardware)
bool fetchXdaNews(String outTitles[XDA_MAX_TITLES], int& outCount);

// New: fetch ALL registered categories; each item has header/link + up to 10 titles
bool fetchXdaNews(xdaNewsItem outItems[], int& outCount, const char* progressMessage, int x = 0, int y = 0);

// Render 3×10 titles with a header that includes last updated time
void renderXdaDevelopersNewsDashboard(EPaper &ep,
                                      const String titles[XDA_MAX_TITLES], int count,
                                      const String &lastUpdatedHHMM);
// NEW: cycle all categories, one screen per category
void renderXdaDevelopersNewsDashboard(EPaper &ep,
                                      const xdaNewsItem items[],
                                      int categoryCount,
                                      const String &lastUpdatedHHMM);