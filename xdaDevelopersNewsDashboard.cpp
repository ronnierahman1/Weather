#include <Arduino.h>
#include "xdaDevelopersNewsDashboard.h"
#include "xda_feeds.h"      // fetchXdaTitles / kXdaFeeds
#include "rssFeedHandler.h" // printHeaderText(..), drawtitle(..), dottedH(..)
#include "text_metrics.h"   // your glyph helpers if needed


#ifndef RENDER_PAUSE_SECONDS
#define RENDER_PAUSE_SECONDS 5
#endif

// ----------------------------------------------------------------------------
// Backward-compatible: fetch a single feed (use a sensible default category).
// You can change XdaCat::PcHardware to any other default you prefer.
// ----------------------------------------------------------------------------
bool fetchXdaNews(String outTitles[XDA_MAX_TITLES], int &outCount)
{
  return fetchXdaTitles(XdaCat::PcHardware, outTitles, XDA_MAX_TITLES, outCount);
}

void reportProgress(EPaper &ep, int idx, int total, const char *what, int x, int y)
{
  ep.fillRect(x, y, 400, 24, TFT_WHITE);
  ep.setTextSize(1);
  ep.setTextColor(TFT_BLACK);
  char buf[64];
  snprintf(buf, sizeof(buf), "%s - Fetched %d/%d ", what, idx + 1, total);
  ep.drawString(buf, x, y);
  ep.update();
}

// ----------------------------------------------------------------------------
// Fetch ALL categories listed in kXdaFeeds. Each outItems[i] is filled with:
//   - headerText (category name)
//   - link       (category URL)
//   - title[]    (up to 10 titles)
// outCount returns the number of categories processed.
// ----------------------------------------------------------------------------
bool fetchXdaNews(xdaNewsItem outItems[], int &outCount, const char *progressMessage, int x, int y)
{
  outCount = 0;
  bool allOk = true;

  for (int i = 0; i < numOfCategorys; ++i)
  {
    const XdaFeedSpec &spec = kXdaFeeds[i];

    // Fetch up to XDA_MAX_TITLES titles for this category
    String titles[XDA_MAX_TITLES], Authors[XDA_MAX_TITLES], Dates[XDA_MAX_TITLES];
    int countTitles = 0;
    bool ok = fetchXdaTitles(static_cast<XdaCat>(i),
                             titles, Authors, Dates, XDA_MAX_TITLES, countTitles);

    // Copy into caller’s struct (even if fetch failed, keep header/link)
    outItems[i].headerText = spec.name;
    outItems[i].link = spec.url;

    for (int t = 0; t < countTitles && t < XDA_MAX_TITLES; ++t)
    {
      outItems[i].title[t] = titles[t];
      outItems[i].Author[t] = Authors[t];
      outItems[i].Date[t] = Dates[t];
    }
    // Clear any unused slots (optional)
    for (int t = countTitles; t < XDA_MAX_TITLES; ++t)
    {
      outItems[i].title[t] = "";
      outItems[i].Author[t] = "";
      outItems[i].Date[t] = "";
    }

    ++outCount;
    allOk = allOk && ok;
    reportProgress(epaper, i, numOfCategorys, progressMessage, x, y);
    delay(50); // small pacing between requests
  }
  return allOk;
}

// Tiny helper to count how many titles are non-empty (<= 10)
static inline int countTitles(const xdaNewsItem &it)
{
  int n = 0;
  while (n < XDA_MAX_TITLES && it.title[n].length())
    ++n;
  return n;
}

// Builds: "XDA Developers - <Category>  | Last Updated: <hh:mm date>"
static inline void buildHeaderLine(String &out,
                                   const String &cat,
                                   const String &lastUpdated)
{
  out.reserve(64 + cat.length() + lastUpdated.length());
  out = F("XDA Developers - ");
  out += cat;
  out += F("  | Last Updated: ");
  out += lastUpdated;
}

// NEW: show each category (up to 10 items) for 30s, then advance
void renderXdaDevelopersNewsDashboard(EPaper &ep, const xdaNewsItem items[], int categoryCount, const String &lastUpdatedHHMM)
{
  // If your EPaper driver exposes width()/height(), use them; otherwise set constants.
  const int WIDTH = 800;  // replace with ep.width() if available
  const int HEIGHT = 480; // replace with ep.height() if available

  const int COLS = 1;
  const int colW = WIDTH;
  const int headH = 24; // fits your printHeaderText(..)
  const int topY = headH + 8;
  const int lineGap = 2;

  // check items[] and if an item.Date in items[] is today, add it to a list of "today" items, check all items and add them in one 1 dimensional array
  // then check for duplicate item.title and remove duplicates
  // String todayItems[XDA_MAX_TITLES * categoryCount]; // max possible today items
  // int todayItemCount = 0;
  // for (int i = 0; i < categoryCount; i++)
  // {
  //   for (int j = 0; j < XDA_MAX_TITLES; j++)
  //   {
  //     // check if item.Date is today's date
  //     // get today's date in the same format as item.Date then compare
  //     DateTime todayDate = parseDateString(getCurrentDateString());
  //     // try to convert item.Date to date object and compare with today's date object
  //     DateTime itemDate = parseDateString(items[i].Date[j]);
  //     if (itemDate == todayDate)
  //     {
  //       // add to list of today items
  //       todayItems[todayItemCount] = items[i].title[j];
  //       todayItemCount++;
  //     }
  //   }
  // }

  // // render the flat list of today items if any
  // if (todayItemCount > 0)
  // {
  //   ep.setRotation(0);
  //   ep.fillScreen(TFT_WHITE);
  //   ep.setTextSize(1);

  //   String hdr;
  //   buildHeaderLine(hdr, "Today", lastUpdatedHHMM);
  //   printHeaderText(ep, hdr, headH);

  //   // 3 columns × up to 10 rows

  //   int idx = 0;
  //   const int x0 = 8;
  //   int y;
  //   while (idx < todayItemCount)
  //   {
  //     y = topY;
  //     for (int i = 0; i < XDA_MAX_TITLES; i++)
  //     {
  //       if(idx >= todayItemCount) break;
  //       char numBuf[6]; // avoids temporary String churn
  //       snprintf(numBuf, sizeof numBuf, "%d. ", idx + 1);
  //       y = printBoldTextLine(ep, numBuf + todayItems[idx], x0, y, colW - 16, lineGap);        
  //       y += 4;        
  //       idx++;
  //     }
  //     ep.update();                          // full refresh per screen
  //     delay(RENDER_PAUSE_SECONDS * 1000UL); // "sleep(30);"
  //     deepClean(ep);                        // prep for next screen      
  //   }
  // }

  for (int i = 0; i < categoryCount; ++i)
  {
    const xdaNewsItem &it = items[i];
    const int count = countTitles(it);

    // Clear and header
    ep.setRotation(0);
    ep.fillScreen(TFT_WHITE);
    ep.setTextSize(1);

    String hdr;
    buildHeaderLine(hdr, it.headerText, lastUpdatedHHMM);
    printHeaderText(ep, hdr, headH);

    // 3 columns × up to 10 rows
    int idx = 0;
    for (int col = 0; col < COLS && idx < count; ++col)
    {
      const int x0 = col * colW + 8;
      int y = topY;

      for (int r = 0; r < XDA_MAX_TITLES && idx < count; ++r, ++idx)
      {
        char numBuf[6]; // avoids temporary String churn
        snprintf(numBuf, sizeof numBuf, "%d. ", idx + 1);
        y = printBoldTextLine(ep, numBuf + it.title[idx], x0, y, colW - 16, lineGap);
        y += 4;
        y = printObliqueTextLine(ep, it.Author[idx] + ", " + it.Date[idx], x0 + 25, y, colW - 16, lineGap);
        // printDottedLine(ep, y, col * colW, (col + 1) * colW);
        y += 4;
      }
    }

    ep.update();                          // full refresh per screen
    delay(RENDER_PAUSE_SECONDS * 1000UL); // "sleep(30);"
    deepClean(ep);                        // prep for next screen
  }
}

// // ----------------------------------------------------------------------------
// // Render 3×10 titles (single list) with a header showing last-updated time.
// // Use this when you’ve fetched a single category into `titles`.
// // ----------------------------------------------------------------------------
// void renderXdaDevelopersNewsDashboard(EPaper &ep,
//                                       const String titles[XDA_MAX_TITLES], int count,
//                                       const String &lastUpdatedHHMM)
// {
//   ep.setRotation(0);
//   ep.fillScreen(TFT_WHITE);

//   // Header
//   const int headH = 24;
//   String header = F("XDA Developers | Last Updated: ");
//   header += lastUpdatedHHMM;
//   printHeaderText(ep, header, headH);

//   // 3 columns × 10 rows
//   const int COLS   = 3;
//   const int colW   = 800 / COLS;       // adapt if your panel width differs
//   const int topY   = headH + 8;
//   const int lineGap= 2;

//   ep.setTextSize(1);

//   int idx = 0;
//   for (int col = 0; col < COLS; ++col) {
//     const int x0 = col * colW + 8;
//     int y = topY;

//     for (int r = 0; r < XDA_MAX_TITLES && idx < count; ++r, ++idx) {
//       String num = String(idx + 1)+". ";
//       y = printNormalTextLine(ep, num + titles[idx], x0, y, colW - 16, lineGap);
//       printDottedLine(ep, y, col * colW, (col + 1) * colW);
//       y += 4;
//     }
//   }

//   ep.update();   // full refresh
// }
