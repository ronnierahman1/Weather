// ui_weather.cpp
// Weather dashboard renderer (optimized, same visuals/values)
// - ZERO layout/value changes
// - Fewer dynamic String temporaries (stack buffers instead)
// - Clear comments, const-correct locals, small helper tweaks

#include <Arduino.h>
#include "globals.h"
#include "text_metrics.h"
#include "icons.h"
#include "weather_data.h"
#include "weather_binding.h"
#include "weather_api.h"
#include "fonts_externs.h"
#include "fonts_data.h"
// #include "Fonts/FreeSansBold8pt7b.h"

// ───────────────────────── Screen geometry (unchanged) ─────────────────────────
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

static constexpr int RULE_Y  = HEADER_Y + HEADER_H;

static constexpr int CARD_W  = 235;     // bottom tiles
static constexpr int CARD_H  = 80;
static constexpr int GAP     = 24;
static constexpr int CARDS_Y = RULE_Y + 20;

static constexpr int RIGHT_COL_X = 540; // Low/High column (right side)


// ───────────────────────── tiny text helpers ─────────────────────────
// These keep the same drawing math while avoiding duplicate code.
static inline void drawL(EPaper& d, const String& s, int x, int y, const GFXfont* f) {
  d.setFreeFont(f); d.drawString(s, x, y);
}
static inline void drawC(EPaper& d, const String& s, int cx, int y, const GFXfont* f) {
  d.setFreeFont(f); d.drawString(s, cx - d.textWidth(s)/2, y);
}
static inline void drawR(EPaper& d, const String& s, int rx, int y, const GFXfont* f) {
  d.setFreeFont(f); d.drawString(s, rx - d.textWidth(s), y);
}

static uint16_t gfxTextWidth(EPaper& d, const String& s) {
  return d.textWidth(s);                 // TFT_eSPI API
}
static uint16_t gfxTextHeight(EPaper& d) {
  return d.fontHeight();                 // height for current font
}
static void drawStringRight(EPaper& d, const String& s, int xRight, int y) {
  d.drawString(s, xRight - (int)d.textWidth(s), y);
}

// Big numeric temperature with a small degree circle (same placement)
static void drawTempBig(EPaper& d, int cx, int baseY, int value) {
  char num[8];
  snprintf(num, sizeof(num), "%d", value);

  d.setFreeFont(&FreeSansBold48pt7b);
  const int w = d.textWidth(num);
  const int x = cx - w/2;
  d.drawString(num, x, baseY);

  // degree-circle offset tuned for FreeSansBold48
  const int dotX = x + w + 10;
  const int dotY = baseY;
  d.drawCircle(dotX, dotY, 6, TFT_BLACK);
  d.drawCircle(dotX, dotY, 5, TFT_BLACK);
  d.drawCircle(dotX, dotY, 4, TFT_BLACK);
}

String getCurrentTime()
{
  struct tm ti;
  if (!getLocalTime(&ti)) return String("--:--");
  char buf[32];
  strftime(buf, sizeof(buf), "%I:%M %p", &ti);
  return String(buf);
}

void getCurrentTime(String& current_time, String & current_Date)
{
  struct tm ti;
  if (!getLocalTime(&ti)) {
    current_time = "--:--";
    current_Date = "--/--/----";
    return;
  }
  char timeBuf[16];
  char dateBuf[16];
  strftime(timeBuf, sizeof(timeBuf), "%I:%M %p", &ti);
  // current date in DD/MM/YYYY and day of the week format like Mon, Tue
  strftime(dateBuf, sizeof(dateBuf), "%d/%m/%Y %a", &ti);
  current_time = String(timeBuf);
  current_Date = String(dateBuf);
  return;
}

// Small pictograms used in the header right column
static inline void iconThermo(EPaper& g, int x, int y) {
  g.drawRoundRect(x, y, 14, 34, 6, TFT_BLACK);
  g.fillCircle(x+7, y+28, 6, TFT_BLACK);
  g.drawFastVLine(x+7, y+6, 18, TFT_BLACK);
}
static inline void iconHumidity(EPaper& g, int x, int y) {
  g.drawLine(x+10, y, x+10, y+18, TFT_BLACK);
  g.drawCircle(x+10, y+22, 8, TFT_BLACK);
}
static void iconSunTiny(EPaper& g, int cx, int cy) {
  // 8 rays at PI/4 increments (kept identical)
  for (int i = 0; i < 8; ++i) {
    const float a = i * PI / 4.0f;
    const int x1 = cx + (int)(8 * cosf(a));
    const int y1 = cy + (int)(8 * sinf(a));
    const int x2 = cx + (int)(14 * cosf(a));
    const int y2 = cy + (int)(14 * sinf(a));
    g.drawLine(x1, y1, x2, y2, TFT_BLACK);
  }
  g.drawCircle(cx, cy, 6, TFT_BLACK);
}

// Dotted horizontal line (same spacing = every 4px)
static inline void dottedH(EPaper& ep, int y, int x0, int x1) {
  for (int x = x0; x < x1; x += 4) ep.drawPixel(x, y, TFT_BLACK);
}


// ───────────────────────── Sections ─────────────────────────
static void printHeaderText(EPaper& ep, const WeatherState& S) {
  // Left: large icon (kept as SUN like your mock)
  IconType icon = iconForWMO(S.currentCode, S.currentIsDay);
  // drawWeatherIcon(icon, LEFT_X + LEFT_W/2, HEADER_Y + 90, 4);
  String current_time, current_Date;
  getCurrentTime(current_time, current_Date);
  // get the first 5 digit of the current time
  String currentTimeShort = current_time.substring(0, 5);
  // get the last 2 digits of the current time
  String currentTimeLong = current_time.substring(6, 8);  

  ep.setFreeFont(&FreeSansBold48pt7b);
  ep.drawString(currentTimeShort, LEFT_X, HEADER_Y );
  int textHeight = gfxTextHeight(ep);
  ep.setFreeFont(&FreeSansBold24pt7b);
  int leftX = SCR_W /3 - gfxTextWidth(ep, currentTimeLong) -PAD;
  ep.drawString(currentTimeLong, leftX, HEADER_Y + textHeight - 20);
  ep.setFreeFont(&FreeSansBold12pt7b);
  ep.drawString(current_Date, LEFT_X, HEADER_Y + textHeight + 20);

  // Center: big temperature + label (identical positions)
  drawTempBig(ep, MID_X + MID_W/2, HEADER_Y + 16, S.temp);
  drawC(ep, "Temperature", MID_X + MID_W/2, HEADER_Y + 100, &FreeSans9pt7b);
  String up = S.dailySunrise[0], dn = S.dailySunset[0];
  // up =  up : String("--:--");
  // dn = (dn.length() >= 16) ? dn.substring(11,16) : String("--:--");

  drawSunriseIcon(MID_X + MID_W/2 - 75, HEADER_Y + 155, 2, 1);
  ep.drawString(up, MID_X + MID_W/2 - 50, HEADER_Y + 140);
  drawSunsetIcon(MID_X + MID_W/2 + 25, HEADER_Y + 155, 2, 1);
  ep.drawString(dn, MID_X + MID_W/2 + 45, HEADER_Y + 140);
  // Vertical separators (exact coordinates)
  ep.drawFastVLine(MID_X - 7,     HEADER_Y - 6, 180, TFT_BLACK);
  ep.drawFastVLine(RIGHT_X - 31,  HEADER_Y - 6, 180, TFT_BLACK);

  // Right facts column: Feels Like / Humidity / Sunny Right Now
  const int xIcon = RIGHT_X;
  const int xTxt  = RIGHT_X + 22;

  // Feels Like
  {
    iconThermo(ep, xIcon, HEADER_Y);
    char buf[16]; snprintf(buf, sizeof(buf), "%d%c", S.feelsLike, 0xB0); // "NN°"
    drawL(ep, String(buf), xTxt, HEADER_Y + 0,  &FreeSansBold9pt7b);
    drawL(ep, "Feels Like", xTxt, HEADER_Y + 20, &FreeSans9pt7b);
  }

  // Humidity
  {
    iconHumidity(ep, xIcon - 5, HEADER_Y + 50);
    char buf[16]; snprintf(buf, sizeof(buf), "%d%%", S.humidity);
    drawL(ep, String(buf), xTxt, HEADER_Y + 50, &FreeSansBold9pt7b);
    drawL(ep, "Humidity",  xTxt, HEADER_Y + 70, &FreeSans9pt7b);
  }

  // Condition now
  iconSunTiny(ep, xIcon + 5, HEADER_Y + 120);
  drawL(ep, S.condition, xTxt, HEADER_Y + 100, &FreeSansBold9pt7b);
  drawL(ep, "Right Now",  xTxt, HEADER_Y + 120, &FreeSans9pt7b);
}

static void drawCards(EPaper& ep, const WeatherState& S) {
  // 2x2 info cards on the left (outlines intentionally commented in original)
  const int x1 = PAD + 6;
  const int y1 = CARDS_Y - 6;

  // Card 1: Partly Cloudy / Today
  IconType icon1 = iconForWMO(S.today.code, S.today.isDay);
  drawWeatherIcon(icon1, x1 + 36, y1 + 50, 2);
  drawL(ep, S.today.phrase, x1 + 84, y1 + 30, &FreeSansBold9pt7b);
  drawL(ep, "Today",        x1 + 84, y1 + 56, &FreeSans9pt7b);

  // vertical hairline between cards
  ep.drawFastVLine(x1 + CARD_W + GAP/2, y1, 80, TFT_BLACK);

  // Card 2: UV High (N)
  
    const int x2 = x1 + CARD_W + GAP;
    const int y2 = y1;
    drawUvIcon(x2 + 36, y2 + 50, 2);
    // drawWeatherIcon(SUN, x2 + 36, y2 + 50, 2);

    char uv[24]; snprintf(uv, sizeof(uv), "%s (%d)", S.today.uvStr.c_str(), S.today.uv);
    drawL(ep, String(uv), x2 + 84, y2 + 30, &FreeSansBold9pt7b);
    drawL(ep, "UV",       x2 + 84, y2 + 56, &FreeSans9pt7b);

    ep.drawFastVLine(x2 + CARD_W + GAP/2, y2, 80, TFT_BLACK);

    // Card 3: Light Rain / Tomorrow
    const int x3 = x1, y3 = y1 + CARD_H + GAP;
    IconType icon2 = iconForWMO(S.tomorrow.code, S.tomorrow.isDay);
    drawWeatherIcon(icon2, x3 + 36, y3 + 50, 2);
    drawL(ep, S.tomorrow.phrase, x3 + 84, y3 + 30, &FreeSansBold9pt7b);
    drawL(ep, "Tomorrow",        x3 + 84, y3 + 56, &FreeSans9pt7b);

    ep.drawFastVLine(x1 + CARD_W + GAP/2, y3, 80, TFT_BLACK);

    // Card 4: UV Moderate (N)
    const int x4 = x2, y4 = y3;
    drawUvIcon(x4 + 36, y4 + 50, 2);
    // drawWeatherIcon(SUN, x4 + 36, y4 + 50, 2);
    char uv2[28]; snprintf(uv2, sizeof(uv2), "%s (%d)", S.tomorrow.uvStr.c_str(), S.tomorrow.uv);
    drawL(ep, String(uv2), x4 + 84, y4 + 30, &FreeSansBold9pt7b);
    drawL(ep, "UV",        x4 + 84, y4 + 56, &FreeSans9pt7b);

    ep.drawFastVLine(x4 + CARD_W + GAP/2, y4, 80, TFT_BLACK);
  
}

static void drawLowHighColumn(EPaper& ep, const WeatherState& S) {
  // re-usable drawer for each "Low/High" pair (thermometer + two labels)
  auto lowHigh = [&](int low, int high, int yBase) {
    const int xr = RIGHT_COL_X;

    // tiny thermometer on the left
    ep.drawRoundRect(xr, yBase - 6, 14, 34, 6, TFT_BLACK);
    ep.fillCircle (xr + 7, yBase + 22, 6, TFT_BLACK);
    ep.drawFastVLine(xr + 7, yBase + 0, 18, TFT_BLACK);

    char lowBuf[12];  snprintf(lowBuf,  sizeof(lowBuf),  "%d%c", low,  0xB0);
    char highBuf[12]; snprintf(highBuf, sizeof(highBuf), "%d%c", high, 0xB0);

    drawL(ep, String(lowBuf),  xr + 26,  yBase - 6, &FreeSansBold9pt7b);
    drawL(ep, "Low",           xr + 26,  yBase + 14, &FreeSans9pt7b);

    drawL(ep, String(highBuf), xr + 100, yBase - 6, &FreeSansBold9pt7b);
    drawL(ep, "High",          xr + 100, yBase + 14, &FreeSans9pt7b);
  };

  // Today / Tomorrow (exact y’s preserved)
  lowHigh(S.today.low,    S.today.high,    CARDS_Y + 22);
  lowHigh(S.tomorrow.low, S.tomorrow.high, CARDS_Y + CARD_H + GAP + 22);
}

static void drawFooter(EPaper& ep, const WeatherState& S) {
  // Dotted footer band (unchanged)
  constexpr int FOOTER_H = 44;
  ep.drawFastHLine(0, SCR_H - FOOTER_H, SCR_W, TFT_BLACK);
  for (int y = SCR_H - FOOTER_H; y < SCR_H; y += 4) dottedH(ep, y, 0, SCR_W);

  drawL(ep, "Weather",   12,         SCR_H - FOOTER_H + 20, &FreeSansBold9pt7b);
  drawR(ep, S.location,  SCR_W - 12, SCR_H - FOOTER_H + 20, &FreeSansBold9pt7b);
}


// ───────────────────────── Public entry ─────────────────────────
void renderWeatherDashboard(EPaper& ep, const WeatherState& S) {
  ep.setTextSize(1);
  // Background clear identical to original
  ep.fillRect(0, 0, SCR_W, SCR_H, TFT_WHITE);

  // Header
  printHeaderText(ep, S);
  ep.drawFastHLine(PAD, RULE_Y, SCR_W - 2*PAD, TFT_BLACK);

  // Cards + right Low/High pairs
  drawCards(ep, S);
  drawLowHighColumn(ep, S);

  // Footer
  drawFooter(ep, S);
}

// -------------------------------- Debug purpose Table Display ------------------------------------
// String getUvString(float uv);

// Small helpers
static String degI(int v) { char b[12]; snprintf(b,sizeof(b),"%d%c", v, 0xB0); return String(b); }
static String pctI(int v) { char b[12]; snprintf(b,sizeof(b),"%d%%", v);      return String(b); }
static String f1(float v) { char b[16]; snprintf(b,sizeof(b),"%.1f", v);      return String(b); }

// left/center/right text with current GFX font
static inline void drawL(EPaper& d, const String& s, int x, int y) { d.drawString(s, x, y); }
static inline void drawR(EPaper& d, const String& s, int rx, int y){ d.drawString(s, rx - d.textWidth(s), y); }

void renderWeatherDataTable(EPaper& ep, const WeatherState& S, int x = 0, int y = 0, int w = SCR_W) {
  // Fonts
  const GFXfont* F_LABEL = &FreeSans9pt7b;
  const GFXfont* F_VAL   = &FreeSansBold9pt7b;

  // Row metrics (baseline-friendly)
  ep.setFreeFont(F_LABEL);
  const int fh   = ep.fontHeight();   // your EPaper wrapper exposes this (like in the Salah UI)
  const int padY = 6;
  const int rowH = fh + padY;         // compact row height

  // Columns
  const int cLabel = x + 8;           // label column
  const int cVal   = x + (w/2);       // value column (left-aligned by default)
  const int cValR  = x + w - 10;      // right edge for right-aligned numbers

  // Drawing cursor
  int yy = y;

  auto row2 = [&](const char* label, const String& valueLeft, const String& valueRight = String()) {

    // label
    ep.setFreeFont(F_LABEL);
    drawL(ep, label, cLabel, yy + fh);

    // left value (bold, small)
    ep.setFreeFont(F_LABEL);
    if (valueLeft.length())  drawL(ep, valueLeft, cVal, yy + fh);

    // optional right-aligned value (good for second metric in same row)
    if (valueRight.length()) drawR(ep, valueRight, cValR, yy + fh);

    yy += rowH;
  };

  // --- “Now” block
  row2("Temperature",   degI(S.temp));
  row2("Feels Like",    degI(S.feelsLike));
  row2("Humidity",      pctI(S.humidity));
  row2("Condition",     S.condition);
  row2("Wind",          f1(S.currentWind));         // units per your API (km/h or m/s)
  row2("Precip (now)",  f1(S.currentPrecip));       // mm or in, depending on API

  // --- “Today” block
  row2("Today L / H",   degI(S.today.low),  degI(S.today.high));
  {
    const String uvText = S.today.uvStr.length() ? S.today.uvStr : getUvString((float)S.today.uv);
    char uvBuf[24]; snprintf(uvBuf, sizeof(uvBuf), "UV %d  %s", S.today.uv, uvText.c_str());
    row2("Today UV", uvBuf);
  }
  row2("Today Cond.",   S.today.phrase);
  // convert S.today.code to string
  char codeBuf[12]; snprintf(codeBuf, sizeof(codeBuf), "%d", S.today.code);
  row2("Today Code",   codeBuf);

  // --- “Tomorrow” block
  row2("Tomorrow L/H",  degI(S.tomorrow.low), degI(S.tomorrow.high));
  {
    const String uvText = S.tomorrow.uvStr.length() ? S.tomorrow.uvStr : getUvString((float)S.tomorrow.uv);
    char uvBuf[24]; snprintf(uvBuf, sizeof(uvBuf), "UV %d  %s", S.tomorrow.uv, uvText.c_str());
    row2("Tomorrow UV", uvBuf);
  }
  row2("Tomorrow",      S.tomorrow.phrase);
  // convert S.tomorrow.code to string
  snprintf(codeBuf, sizeof(codeBuf), "%d", S.tomorrow.code);
  row2("Tomorrow Code", codeBuf);

  // --- Footer bits
  row2("Location",      S.location);
  row2("Updated",       S.updated);
}

