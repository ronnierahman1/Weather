// TheScienceDashboard.cpp
// The Scientist (Atom) dashboard — mirrors scienceDailyDashboard style.

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include "ui.h"
#include "config.h"
#include "text_metrics.h"
#include "icons.h"
#include "gfx_metrics.h"
#include "fonts_data.h"
#include "rssFeedHandler.h" // htmlDecode, extractTag, drawtitle, printHeaderText, httpGET_Insecure
#include "theScientistDashboard.h"
#include "Fonts/FreeSans8pt7b.h"
#include "newsFeeds.h"


bool fetchTheScientistNewsFromPi(String outItems[numberOfTheScientistNewsItems], int &outCount)
{
  outCount = 0;

  // Change this if your Pi IP/port differs
  String url = String(PI_BASE_URL) + "/api/the_scientist";

  WiFiClient client;
  HTTPClient http;

  if (!http.begin(client, url))
    return false;

  const int code = http.GET();
  if (code != HTTP_CODE_OK)
  {
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();

  // Keep it modest for ESP32-C3 RAM
  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err)
    return false;

  JsonArray items = doc["items"].as<JsonArray>();
  if (items.isNull())
    return false;

  int n = 0;
  for (JsonVariantConst v : items)
  {
    if (n >= numberOfTheScientistNewsItems) break;
    const char *s = v.as<const char *>();
    if (s && *s) outItems[n++] = String(s);
  }

  outCount = n;
  return outCount > 0;
}

// // ---- Keep public API name, but route it to Pi now ----
// bool fetchTheScientistNews(String outItems[numberOfTheScientistNewsItems], int &outCount)
// {
//   return fetchTheScientistNewsFromPi(outItems, outCount);
// }

// void renderTheScientistNewsDashboard(EPaper &ep,
//                                     const String items[numberOfTheScientistNewsItems],
//                                     int count,
//                                     const String &lastUpdatedHHMM, PageRenderMode mode)
// {
//   renderNewsDashboard(ep, items, count, lastUpdatedHHMM, mode, 1, "The Scientist");
// }

// Public: fetch up to 70 items from The Scientist Atom feed
bool fetchTheScientistNews(String outItems[numberOfTheScientistNewsItems], int &outCount)
{
  outCount = 0;
  String body;
  if (!httpGET_Insecure(F("https://www.the-scientist.com/atom/latest"), body))
  {
    return false;
  }
  outCount = parseAtomEntries(body, outItems, numberOfTheScientistNewsItems, "");
  return outCount > 0;
}

void renderTheScientistNewsDashboard(EPaper &ep,
                                     const String items[numberOfTheScientistNewsItems],
                                     int count,
                                     const String &lastUpdatedHHMM, PageRenderMode mode = Full_Page)
{
  renderNewsDashboard(ep, items, count, lastUpdatedHHMM, mode, 1, "The Scientist");
}

// Public: render (paged like ScienceDaily; titles only)
// void renderTheScientistNewsDashboard(EPaper &ep,
//                                      const String items[numberOfTheScientistNewsItems],
//                                      int count,
//                                      const String &lastUpdatedHHMM, PageRenderMode mode = Full_Page)
// {
//   ep.setRotation(0);
//   ep.fillScreen(TFT_WHITE);
//   ep.setTextSize(1);

//   const int HEAD_SZ = 2;
//   const int headH = charH(HEAD_SZ) + 8;

//   // Header text
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
//   String header = F("The Scientist - Latest - Last Updated: ");
//   header += dt;

//   // Layout (mirror ScienceDaily dashboard)
//   const int COLS = 1, ROWS = 20;
//   const int colW = 800 / COLS;
//   const int topY = headH + 10;
//   const int itemGap = 3;
//   const int lineGap = 1;

//   int maxCol = ceil(count / (float)ROWS);
//   for (int col = 0; col < maxCol; ++col)
//   {
//     int x = 8;
//     int y = topY;

//     printHeaderText(ep, header, headH); // uses FreeSansBold9pt7b and draws the rule

//     for (int r = 0; r < ROWS; ++r)
//     {
//       int idx = col * ROWS + r;
//       if (idx >= count)
//         break;

//       // Numbered title (bold), description optional (kept off to match ScienceDaily look)
//       String num = String(idx + 1) + F(". ");
//       y = printBoldTextLine(ep, num + items[idx], x, y, colW - 16, lineGap);
//       // If you later want summaries, uncomment:
//       // y = printNormalTextLine(ep, items[idx][1], x, y, colW - 16, lineGap);

//       y += itemGap;
//     }

//     ep.update(); // full frame update
//     if (col < maxCol - 1)
//     {
//       // same behavior as your ScienceDaily page-turn
//       sleep(30);
//       deepClean(ep);
//     }
//   }
// }
