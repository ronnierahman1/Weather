#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "ui.h"
#include "config.h"
#include "text_metrics.h"
#include "icons.h"
#include "hackerNewsDashboard.h"
#include "gfx_metrics.h"
#include "fonts_data.h"
#include "rssFeedHandler.h"
#include "HtmlMarkupDecode.h"
#include "newsFeeds.h"
#include <ArduinoJson.h>


// Extract <span class="titleline"><a ...>Title</a>…>   (current HN markup)
// Falls back to legacy <a class="storylink">Title</a>
static int parseTitles(const String &html, String out[numberOfNewsItems])
{
  int count = 0;
  int pos = 0;

  // Primary parser (modern HN)
  while (count < numberOfNewsItems)
  {
    int s = html.indexOf(F("<span class=\"titleline\">"), pos);
    if (s < 0)
      break;
    int a1 = html.indexOf('>', html.indexOf(F("<a "), s)); // end of <a ...>
    int a2 = html.indexOf(F("</a>"), a1 + 1);
    if (a1 < 0 || a2 < 0)
    {
      pos = s + 1;
      continue;
    }
    String title = htmlDecode(html.substring(a1 + 1, a2));
    title.trim();
    if (title.length())
      out[count++] = title;
    pos = a2 + 4;
    if (count >= numberOfNewsItems)
      break;
  }

  // Fallback (older markup)
  if (count == 0)
  {
    pos = 0;
    while (count < numberOfNewsItems)
    {
      int a = html.indexOf(F("<a class=\"storylink\""), pos);
      if (a < 0)
        break;
      int a1 = html.indexOf('>', a);
      int a2 = html.indexOf(F("</a>"), a1 + 1);
      if (a1 < 0 || a2 < 0)
      {
        pos = a + 1;
        continue;
      }
      String title = htmlDecode(html.substring(a1 + 1, a2));
      title.trim();
      if (title.length())
        out[count++] = title;
      pos = a2 + 4;
    }
  }
  return count;
}

bool fetchHackerNews(String outTitles[numberOfNewsItems], int &outCount)
{
  outCount = 0;
  String body;
  if (!httpGET_Insecure(F("https://news.ycombinator.com/"), body))
    return false;
  outCount = parseTitles(body, outTitles);
  return outCount > 0;
}

static bool httpGET_HTTP(const String& url, String& outBody)
{
  outBody = "";

  if (WiFi.status() != WL_CONNECTED) {
    // Serial.println("httpGET_HTTP: WiFi not connected");
    return false;
  }

  WiFiClient client;          // <-- IMPORTANT: non-secure client for http://
  HTTPClient http;

  // Serial.println("HTTP GET: " + url);

  if (!http.begin(client, url)) {
    // Serial.println("http.begin() failed");
    return false;
  }

  http.setTimeout(20000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.addHeader("Accept", "application/json");

  int httpCode = http.GET();
  // Serial.print("HTTP code: ");
  // Serial.println(httpCode);

  if (httpCode <= 0) {
    // Serial.print("HTTP error: ");
    // Serial.println(http.errorToString(httpCode));
    http.end();
    return false;
  }

  outBody = http.getString();
  // Serial.print("Body length: ");
  // Serial.println(outBody.length());

  http.end();
  return (httpCode >= 200 && httpCode < 300);
}
bool fetchHackerNewsFromPi(String outTitles[numberOfNewsItems], int &outCount)
{
  outCount = 0;
  for (int i = 0; i < numberOfNewsItems; ++i) outTitles[i] = "";

  String body;

  // Configure your PI base URL in config.h, e.g.:
  // #define PI_BASE_URL "http://192.168.1.10:8088"
  String url = String(PI_BASE_URL) + "/api/hackernews";

  // Serial.println("Fetching HN from Pi URL: " + url);
  if (!httpGET_HTTP(url, body)) {
    // // Serial.println("Failed to fetch HN from Pi");
    return false;
  }

  StaticJsonDocument<6144> doc;
  DeserializationError err = deserializeJson(doc, body);

  if (err) {
    // // Serial.println("Failed to parse JSON from Pi");
    return false;
  }

  JsonArray items = doc["items"].as<JsonArray>();
  if (items.isNull()) {
    // Serial.println("No items array in JSON from Pi");
    return false;
  }

  int i = 0;
  for (JsonVariant v : items) {
    if (i >= numberOfNewsItems) break;
    const char* s = v.as<const char*>();
    if (s && *s) outTitles[i++] = String(s);
  }
  outCount = i;
  // Serial.println("Fetched " + String(outCount) + " HN items from Pi");
  // Serial.println(outTitles[0]);
  // if (outLastUpdatedHHMM) {
  //   const char* hhmm = doc["last_updated_hhmm"] | "";
  //   *outLastUpdatedHHMM = String(hhmm);
  // }

  return outCount > 0;
}

// Simple word-wrap drawer for a fixed column width
static int drawWrapped(EPaper &ep, const String &prefixNum,
                       const String &text, int x, int y,
                       int colW, int textSize, int lineGap = 2)
{
  ep.setTextSize(textSize);

  // Compose "N. Title"
  String s = prefixNum + text;

  // Wrap by words to fit colW
  int cw = charW(textSize), ch = charH(textSize);
  int maxChars = max(1, (colW - 2) / cw);
  int lineY = y;

  int start = 0;
  while (start < s.length())
  {
    int end = start;
    int lastSpace = -1;
    int taken = 0;
    while (end < s.length() && taken < maxChars)
    {
      char c = s[end];
      if (c == ' ')
        lastSpace = end;
      end++;
      taken++;
    }
    int cut = (end == s.length() || s[end] == ' ') ? end : (lastSpace > start ? lastSpace : end);
    String line = s.substring(start, cut);
    ep.drawString(line, x, lineY);
    lineY += ch + lineGap + 5;
    start = (cut == end) ? end : cut + 1; // skip space
  }
  return lineY; // next y
}

void renderHackerNewsDashboard(EPaper &ep,
                               const String titles[numberOfNewsItems], int count,
                               const String &lastUpdatedHHMM, PageRenderMode mode = Full_Page)
{
  renderNewsDashboard(ep, titles, count, lastUpdatedHHMM, mode, 2, "Hacker News");
}

// void renderHackerNewsDashboard(EPaper &ep,
//                                const String titles[numberOfHackerNewsItems], int count,
//                                const String &lastUpdatedHHMM, PageRenderMode mode = Full_Page)
// {

//   // full-screen render; honor your project rules
//   ep.setRotation(0);

//   // ep.fillScreen(TFT_WHITE);
//   ep.setTextSize(1);

//   // Header
//   int headerY = 0;
//   int startIdx = 0, endIdx = count;
//   if (mode == Full_Page)
//   {
//     headerY = 0;
//     startIdx = 0;
//     endIdx = count;
//   }
//   else if (mode == First_Page)
//   {
//     headerY = 240;
//     startIdx = 0;
//     endIdx = min(count, 9);
//   }
//   else if (mode == Second_Page)
//   {
//     headerY = 240;
//     startIdx = 10;
//     endIdx = min(count, 19);
//   }

//   const int HEAD_SZ = 2;
//   const int headH = charH(HEAD_SZ) + headerY + 18;

//   ep.drawFastHLine(0, headH, 800, TFT_BLACK);

//   // Build header text
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
//   ep.setFreeFont(&FreeSansBold9pt7b);
//   String header = F("Hacker news - Last Updated: ");
//   header += dt;
//   header += (mode == Full_Page ? F("") : (mode == First_Page ? F(" - Page 1") : F(" - Page 2")));
  
//   ep.drawString(header, 10, headH - 18);

//   // Columns
//   const int COLS = 2;
//   int rows = (mode == Full_Page) ? 10 : 5;
//   const int colW = 800 / COLS;
//   const int topY = headH + 10;
//   const int BOT = 480 - 6;

//   // Optional vertical separators
//   ep.drawFastVLine(colW, topY - 8, BOT - topY + 8, TFT_BLACK);
//   // ep.drawFastVLine(colW*2,topY - 8, BOT - topY + 8, TFT_BLACK);

//   const int itemSize = 1;
//   const int lineGap = 2;

//   ep.setFreeFont(&FreeSans8pt7b);

//   // We try to keep each column ~equal height by spacing items within the column
//   for (int col = 0; col < COLS; col++)
//   {
//     int x = col * colW + 8;
//     int y = topY;
//     for (int r = 0; r < rows; r++)
//     {
//       // Serial.println("Rendering item " + String(r + 1));
//       int idx = col * rows + r + startIdx;
//       // if (idx >= count)
//       //   break;
//       String num = String(idx + 1) + F(". ");
//       y = drawWrapped(ep, num, titles[idx], x, y, colW - 16, itemSize, lineGap);
//       y += 4; // small gap between entries
//     }
//   }

//   // Push full-frame to the panel
//   ep.update(); // full refresh: copies the sprite to panel and triggers EPD update
// }
