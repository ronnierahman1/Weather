#include <Arduino.h>
#include <math.h>
#include "globals.h"
#include "text_metrics.h"
#include "icons.h"
#include <WiFi.h>
#include "weather_data.h"
#include "weather_binding.h"
#include "weather_api.h"
#include "fonts_externs.h"
#include "fonts_data.h"
#include "WeatherGraph.h"

// getCurrentTime is defined in ui_weather.cpp; we just declare it here.
void getCurrentTime(String& current_time, String& current_Date);

// ───────────────────────── Screen geometry ─────────────────────────
static constexpr int SCR_W = 800;
static constexpr int SCR_H = 480;

static constexpr int PAD      = 18;
static constexpr int HEADER_Y = 24;     // top padding
static constexpr int HEADER_H = 190;    // header band height

static constexpr int LEFT_X = PAD;      // big icon block
static constexpr int LEFT_W = 240;

static constexpr int MID_X  = 280;      // big temp block
static constexpr int MID_W  = 220;

static constexpr int RIGHT_X = 560;     // right facts column
static constexpr int RIGHT_W = SCR_W - RIGHT_X - PAD;

static constexpr int RULE_Y  = HEADER_Y + HEADER_H; //HEADER_H = 190;HEADER_Y = 24;

// Graph area (occupies most of the lower half)
static constexpr int GRAPH_PAD_X = PAD;
static constexpr int GRAPH_PAD_Y = 26;
static constexpr int GRAPH_H     = 180;   // height of plotting area (excluding labels)

// Footer band
static constexpr int FOOTER_H    = 44;

// ───────────────────────── Tiny text helpers ─────────────────────────
static inline void drawL(EPaper& d, const String& s, int x, int y, const GFXfont* f) {
  d.setFreeFont(f);
  d.drawString(s, x, y);
}

static inline void drawC(EPaper& d, const String& s, int cx, int y, const GFXfont* f) {
  d.setFreeFont(f);
  d.drawString(s, cx - d.textWidth(s) / 2, y);
}

static inline void drawR(EPaper& d, const String& s, int rx, int y, const GFXfont* f) {
  d.setFreeFont(f);
  d.drawString(s, rx - d.textWidth(s), y);
}

static String degI(int v) {
  char b[12];
  snprintf(b, sizeof(b), "%d%c", v, 0xB0);
  return String(b);
}

static String pctI(int v) {
  char b[12];
  snprintf(b, sizeof(b), "%d%%", v);
  return String(b);
}

static String f1(float v) {
  if (!isfinite(v)) return String("--");
  char b[16];
  snprintf(b, sizeof(b), "%.1f", v);
  return String(b);
}

// ───────────────────────── "Today" header ─────────────────────────
static void drawHeader(EPaper& ep, const WeatherState& S) {
  // clear header band
  ep.fillRect(0, 0, SCR_W, RULE_Y, TFT_WHITE);

  // Left: large icon + stacked facts (Sunrise / Sunset / Humidity / Wind / Precip)
  const int iconCx = LEFT_X + LEFT_W / 2 - 20;
  const int iconCy = HEADER_Y + 50;

  IconType icon = iconForWMO(S.currentCode, S.currentIsDay);
  drawWeatherIcon(icon, iconCx, iconCy, 3);

  const int factsX = LEFT_X;
  int y = HEADER_Y + 190;
  const int lineH = 22;

  auto fact = [&](const char* label, const String& value) {
    drawL(ep, String(label), factsX, y, &FreeSans9pt7b);
    y += lineH;
    drawL(ep, value, factsX, y, &FreeSansBold9pt7b);
    y += lineH;
  };

  // dailySunrise/dailySunset index 0 = today, strings "YYYY-MM-DDTHH:MM"
  String sunrise("--:--"), sunset("--:--");
  if (S.dailyCount > 0) {
    sunrise = S.dailySunrise[0];
    sunset  = S.dailySunset[0];
  }

  fact("Sunrise", sunrise);
  fact("Sunset",  sunset);
  fact("Humidity", pctI(S.humidity));
  fact("Wind",     f1(S.currentWind));
  fact("Precip",   f1(S.currentPrecip));

  // Middle: big temperature with "Feels like" just under it
  const int midCx = MID_X + MID_W / 2;
  const int baseY = HEADER_Y + 0;

  {
    char num[8];
    snprintf(num, sizeof(num), "%d", S.temp);

    ep.setFreeFont(&FreeSansBold24pt7b);
    const int w = ep.textWidth(num);
    const int x = 220 - w / 2; //x= midCx - w / 2;
    ep.drawString(num, x, baseY);

    // Degree symbol + C
    const int degX = x + w + 15;
    const int degY = baseY + 10;
    ep.setFreeFont(&FreeSansBold18pt7b);
    ep.drawString(String((char)0xB0) + "C", degX, degY);

    // Feels like
    char buf[32];
    snprintf(buf, sizeof(buf), "Feels like %d%c", S.feelsLike, 0xB0);
    drawC(ep, String(buf), x + 55, baseY + 50, &FreeSans9pt7b);
  }

  // Right: location + current date, and Today/Tomorrow highs/lows
  String current_time, current_date;
  getCurrentTime(current_time, current_date);

  drawR(ep, S.location, SCR_W - PAD, HEADER_Y, &FreeSansBold12pt7b);
  drawR(ep, current_date, SCR_W - PAD, HEADER_Y + 28, &FreeSans9pt7b);

  int colX = RIGHT_X;
  int colY = HEADER_Y + 50;
  const int colLH = 22;

  auto hiLo = [&](const char* label, const MiniDay& day) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d%c / %d%c",
             day.low, 0xB0, day.high, 0xB0);
    drawL(ep, String(label), colX, colY, &FreeSans9pt7b);
    drawR(ep, String(buf), RIGHT_X + RIGHT_W, colY, &FreeSansBold9pt7b);
    colY += colLH;
  };

  hiLo("Today",    S.today);
  hiLo("Tomorrow", S.tomorrow);

  // Draw Wi‑Fi icon at top-right of the header band when Wi‑Fi is enabled
  int fh = 0;
#if defined(TFT_eSPI_VERSION) || defined(SEEED_GFX_H) || defined(ARDUINO_TFT_ESPI)
  fh = ep.fontHeight();
#else
  fh = glyphHeight(12);
#endif
  int iconH = fh / 4;
  iconH = 20;
  const int iconLeft = SCR_W - PAD - iconH;
  const int iconTop = HEADER_Y -30; // slightly below top padding
  if (WiFi.getMode() != WIFI_OFF) drawWifiIcon(iconLeft, iconTop, iconH);
}

// ───────────────────────── Daily forecast strip ─────────────────────────
static void drawDailyStrip(EPaper& ep, const WeatherState& S) {
  // Single row of small day-forecast tiles, like the Hackaday example.
  const int maxDays = 5;
  const int days = min(S.dailyCount, maxDays);
  if (days <= 0) return;

  const int yTop  = HEADER_Y + HEADER_H - 100; // yTop = 120 + 180 - 50 = 250
  const int yIcon = yTop + 12;
  const int cellW = (SCR_W - 2 * PAD) / days;

  for (int i = 0; i < days; ++i) {
    int idx = i;  // include today; use (i+1) if you prefer to start from tomorrow
    if (idx >= S.dailyCount) break;

    const int cx = PAD + cellW * i + cellW / 2;

    String day = S.dailyDayAbbr[idx];
    drawC(ep, day, cx, yTop, &FreeSansBold9pt7b);

    IconType icon = iconForWMO(S.dailyCode[idx], true);
    drawWeatherIcon(icon, cx, yIcon+20, 1);

    char buf[24];
    snprintf(buf, sizeof(buf), "%d/%d",
             (int)roundf(S.dailyMax[idx]),
             (int)roundf(S.dailyMin[idx]));
    drawC(ep, String(buf), cx, yIcon + 50, &FreeSans9pt7b);
  }
}

// ───────────────────────── Graph helpers ─────────────────────────

// Map a value in [vMin, vMax] to Y in screen coordinates (0 is top screen).
static int mapY(float value, float vMin, float vMax, int yTop, int yBottom) {
  if (!isfinite(value)) return yBottom;
  if (vMax <= vMin + 0.001f) {
    return (yTop + yBottom) / 2;
  }
  float t = (value - vMin) / (vMax - vMin);
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  return yBottom - (int)((yBottom - yTop) * t + 0.5f);
}

// Extract today's hourly temperatures and min/max.
static int collectTodayTemps(const WeatherState& S,
                             float* outTemps,
                             String* outLabels,
                             const int maxPoints) {
  int n = min(S.hourlyCount, maxPoints);
  if (n <= 0) return 0;

  int used = 0;
  for (int i = 0; i < n; ++i) {
    outTemps[used]  = S.hourlyTemp[i];

    // Label: keep just hour "HH" from "YYYY-MM-DDTHH:MM"
    String t = S.hourlyTime[i];
    int tIdx = t.indexOf('T');
    if (tIdx >= 0 && tIdx + 3 <= (int)t.length()) {
      outLabels[used] = t.substring(tIdx + 1, tIdx + 3) + "h";
    } else {
      outLabels[used] = t;
    }
    ++used;
  }
  return used;
}

static void drawTemperatureGraph(EPaper& ep, const WeatherState& S) {
  // Graph rectangle
  const int left_pad = 100;
  const int gx = GRAPH_PAD_X; //18
  const int gy = RULE_Y + GRAPH_PAD_Y - 30;//gy = 216; 
  const int gw = SCR_W - 2 * GRAPH_PAD_X - left_pad; //gw = 800 - 36 - 100 = 664 
  const int gh = GRAPH_H;// GRAPH_H     = 180;

  const int xLeft  = gx + left_pad; // xLeft = 18 + 100 = 118
  const int xRight = gx + gw; // xRight = 800 - 18 = 782
  const int yTop   = gy; // yTop = 216
  const int yBot   = gy + gh; // yBot = 216 + 180 = 396

  // Frame
  ep.drawRect(xLeft, yTop, gw, gh, TFT_BLACK);

  constexpr int MAX_PTS = WeatherState::MAX_HOURLY;
  float temps[MAX_PTS];
  String labels[MAX_PTS];

  const int n = collectTodayTemps(S, temps, labels, MAX_PTS);
  if (n <= 0) return;

  float tMin = temps[0];
  float tMax = temps[0];
  for (int i = 1; i < n; ++i) {
    if (temps[i] < tMin) tMin = temps[i];
    else if (temps[i] > tMax) tMax = temps[i];
  }

  if (tMin == tMax) {
    tMin -= 1.0f;
    tMax += 1.0f;
  } else {
    const float pad = (tMax - tMin) * 0.15f;
    tMin -= pad;
    tMax += pad;
  }

  // Y-axis labels (min / max at left)
  {
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f%c", tMax, 0xB0);
    drawL(ep, String(buf), xLeft - 24, yTop + 18, &FreeSans9pt7b);

    snprintf(buf, sizeof(buf), "%.0f%c", tMin, 0xB0);
    drawL(ep, String(buf), xLeft - 24, yBot - 15, &FreeSans9pt7b);
  }

  // Horizontal grid lines at min, mid, max
  {
    const float mid = 0.5f * (tMin + tMax);
    const int yMid = mapY(mid, tMin, tMax, yTop, yBot);

    ep.drawFastHLine(xLeft, yTop,  gw, TFT_LIGHTGREY);
    ep.drawFastHLine(xLeft, yMid,  gw, TFT_LIGHTGREY);
    ep.drawFastHLine(xLeft, yBot,  gw, TFT_LIGHTGREY);
  }

  // Polyline of temps
  for (int i = 0; i < n - 1; ++i) {
    const int x0 = xLeft + (int)((gw - 10) * (float)i / (float)(n - 1)) + 5;
    const int x1 = xLeft + (int)((gw - 10) * (float)(i + 1) / (float)(n - 1)) + 5;

    const int y0 = mapY(temps[i],     tMin, tMax, yTop, yBot);
    const int y1 = mapY(temps[i + 1], tMin, tMax, yTop, yBot);

    ep.drawLine(x0, y0, x1, y1, TFT_BLACK);
  }

  // Points + labels for first/middle/last (and every 3rd when few points)
  for (int i = 0; i < n; ++i) {
    const int x = xLeft + (int)((gw - 10) * (float)i / (float)(n - 1)) + 5;
    const int y = mapY(temps[i], tMin, tMax, yTop, yBot);

    ep.fillCircle(x, y, 2, TFT_BLACK);

    bool labelThis = (i == 0) || (i == n - 1) || (i == n / 2);
    if (n <= 12 && (i % 3 == 0)) labelThis = true;

    if (labelThis) {
      drawC(ep, labels[i], x, yBot + 14, &FreeSans9pt7b);
    }
  }

  // Title above graph
//   drawL(ep, "Today temperature graph", xLeft, yTop - 12, &FreeSansBold12pt7b);
}

// ───────────────────────── Footer ─────────────────────────
static void drawFooter(EPaper& ep, const WeatherState& S) {
  const int yBand = SCR_H - FOOTER_H;

  ep.drawFastHLine(0, yBand, SCR_W, TFT_BLACK);
  drawL(ep, "Weather", PAD, yBand + 16, &FreeSansBold9pt7b);
  drawR(ep, S.location, SCR_W - PAD, yBand + 16, &FreeSansBold9pt7b);
}

// ───────────────────────── Public entry ─────────────────────────
void renderWeatherGraphDashboard(EPaper& ep, const WeatherState& S) {
  ep.setRotation(0);           // project-wide convention
  ep.setTextSize(1);
  ep.fillRect(0, 0, SCR_W, SCR_H, TFT_WHITE);

  drawHeader(ep, S);
  drawDailyStrip(ep, S);
  ep.drawFastHLine(PAD, RULE_Y-10, SCR_W - 2 * PAD, TFT_BLACK);

  drawTemperatureGraph(ep, S);
  drawFooter(ep, S);

  ep.update();                 // ep.display() doesn't exist
}
