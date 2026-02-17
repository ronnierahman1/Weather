// scienceDailyDashboard.cpp
#include <Arduino.h>
#include "scienceDailyDashboard.h"
#include "newsFeeds.h"
#include "pi_feed_client.h"   // your helper that fetches JSON list from Pi
#include "config.h"


static const String PI_SCIDAILY_URL = String(PI_BASE_URL) + "/api/sciencedaily";

bool fetchScienceDailyNewsFromPi(String outItems[numberOfScienceDailyNewsItems], int& outCount)
{
  return fetchStringListFromPi(
    PI_SCIDAILY_URL,
    "items",
    outItems,
    numberOfScienceDailyNewsItems,
    outCount,
    nullptr
  );
}

bool fetchScienceDailyNews(String outItems[numberOfScienceDailyNewsItems], int& outCount)
{
  outCount = 0;
  String body;

  // (If you still want direct fetching as fallback)
  if (!httpGET_Insecure(F("https://www.sciencedaily.com/rss/top.xml"), body))
    return false;

  outCount = parseRssTitles(body, outItems, numberOfScienceDailyNewsItems, " - Science Daily");
  return outCount > 0;
}

void renderScienceDailyNewsDashboard(EPaper& ep,
                                    const String items[numberOfScienceDailyNewsItems],
                                    int count,
                                    const String& lastUpdatedHHMM,
                                    PageRenderMode mode)
{
  renderNewsDashboard(ep, items, count, lastUpdatedHHMM, mode, 1, "Science Daily");
}
// // Public: render 3×10 dashboard
// void renderScienceDailyNewsDashboard(EPaper &ep,
//                                      const String items[numberOfScienceNewsItems], int count,
//                                      const String &lastUpdatedHHMM = "", PageRenderMode mode = Full_Page)
// {
//   ep.setRotation(0);
//   ep.fillScreen(TFT_WHITE);

//   // Header
//   const int HEAD_SZ = 2;
//   const int headH = charH(HEAD_SZ) + 8;
//   ep.setTextSize(1);
//   ep.setFreeFont(&FreeSansBold9pt7b);

//   String dt;
//   if (lastUpdatedHHMM.length())
//   {
//     dt = lastUpdatedHHMM;
//   }
//   else
//   {
//     struct tm ti{};
//     if (getLocalTime(&ti))
//     {
//       char b1[6];
//       strftime(b1, sizeof(b1), "%H:%M", &ti);
//       char b2[20];
//       strftime(b2, sizeof(b2), "%d %b %Y", &ti);
//       dt = String(b1) + ", " + String(b2);
//     }
//     else
//       dt = F("--:--, -- --- ----");
//   }

//   String header = F("Science Daily Top News - Last Updated: ");
//   header += dt;

//   // 3 columns × 10 items
//   const int COLS = 1, ROWS = 20;
//   const int colW = 800 / COLS;
//   const int topY = headH + 10;
//   const int BOT = 480 - 6;
//   const int itemSize = 1;
//   const int lineGap = 1;
//   int maxCol = ceil(count / (float)ROWS);
//   for (int col = 0; col < maxCol; ++col)
//   {
//     int x = 8;
//     int y = topY;
//     printHeaderText(ep, header, headH);
//     for (int r = 0; r < ROWS; ++r)
//     {
//       int idx = col * ROWS + r;
//       if (idx >= count)
//         break;
//       String num = String(idx + 1) + F(". ");
//       y = printBoldTextLine(ep, num + items[idx], x, y, colW - 16, lineGap);
//       y += 3;
//     }
//     ep.update(); // full frame update
//     if (col < maxCol - 1)
//     {
//       sleep(30);
//       deepClean(ep);
//     }
//   }
// }
