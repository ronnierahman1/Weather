
#include <Arduino.h>
#include "newsFeeds.h"
#include "globals.h"
#include "fonts_data.h"
// #include "makkah_small.h"

static inline int charW(int sz) { return 7 * sz; }
static inline int charW2(int sz) { return 5 * sz; }
static inline int charH(int sz) { return 10 * sz; }
static inline int charH2(int sz) { return 8 * sz; }
void deepCleanBottomHalf    (EPaper& epaper);
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


void renderNewsDashboard(EPaper &ep,
                               const String titles[numberOfNewsItems], int count,
                               const String &lastUpdatedHHMM, PageRenderMode mode = Full_Page, int cols = 2, String titlePrefix = "News")
{
    if (cols > 2)
        cols = 2;
    else if (cols < 1)
        cols = 1;
    if(mode ==PageRenderMode::Second_Page)
    {
        deepCleanBottomHalf(ep);
    }
    // full-screen render; honor your project rules
    ep.setRotation(0);

    // ep.fillScreen(TFT_WHITE);
    ep.setTextSize(1);

    // Header
    int headerY = 0;
    int startIdx = 0, endIdx = count;
    if (mode == Full_Page)
    {
        headerY = 0;
        startIdx = 0;
        endIdx = count;
    }
    else if (mode == First_Page)
    {
        headerY = 240;
        startIdx = 0;
        endIdx = min(count, 9);
    }
    else if (mode == Second_Page)
    {
        headerY = 240;
        startIdx = 10;
        endIdx = min(count, 19);
    }

    const int HEAD_SZ = 2;
    const int headH = charH(HEAD_SZ) + headerY + 18;
    
    ep.drawFastHLine(0, headH, 800, TFT_BLACK);
    // if(mode != Full_Page)
    // {
    //     ep.update(620, 45, 180, 180, (uint16_t*)gImage_makkah_small);  // Makkah image
    // }
    // Build header text
    String dt;
    if (lastUpdatedHHMM.length())
    {
        dt = lastUpdatedHHMM;
    }
    else
    {
        struct tm ti{};
        if (getLocalTime(&ti))
        {
            char b1[6];
            strftime(b1, sizeof(b1), "%H:%M", &ti);
            char b2[20];
            strftime(b2, sizeof(b2), "%d %b %Y", &ti);
            dt = String(b1) + ", " + String(b2);
        }
        else
            dt = F("--:--, -- --- ----");
    }
    ep.setFreeFont(&FreeSansBold9pt7b);
    String header = titlePrefix + F(" - Last Updated: ");
    header += dt;
    header += (mode == Full_Page ? F("") : (mode == First_Page ? F(" - Page 1") : F(" - Page 2")));

    ep.drawString(header, 10, headH - 18);

    // Columns
    const int COLS = 2;
    int rows = (mode == Full_Page) ? 10 : 5;
    const int colW = 800 / COLS;
    const int topY = headH + 10;
    const int BOT = 480 - 6;

    // Optional vertical separators
    ep.drawFastVLine(colW, topY - 8, BOT - topY + 8, TFT_BLACK);
    // ep.drawFastVLine(colW*2,topY - 8, BOT - topY + 8, TFT_BLACK);

    const int itemSize = 1;
    const int lineGap = 2;

    ep.setFreeFont(&FreeSans8pt7b);

    // We try to keep each column ~equal height by spacing items within the column
    for (int col = 0; col < COLS; col++)
    {
        int x = col * colW + 8;
        int y = topY;
        for (int r = 0; r < rows; r++)
        {
            // Serial.println("Rendering item " + String(r + 1));
            int idx = col * rows + r + startIdx;
            // if (idx >= count)
            //   break;
            String num = String(idx + 1) + F(". ");
            y = drawWrapped(ep, num, titles[idx], x, y, colW - 16, itemSize, lineGap);
            y += 4; // small gap between entries
        }
    }

    // Push full-frame to the panel
    ep.update(); // full refresh: copies the sprite to panel and triggers EPD update
}

void deepCleanBottomHalf    (EPaper& epaper) { epaper.fillRect(0, 240, 800, 240, TFT_BLACK); epaper.update();
                                     epaper.fillRect(0, 240, 800, 240, TFT_WHITE); epaper.update();
                                     delay(10); }
