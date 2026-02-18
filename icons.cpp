#include <math.h>
#include "icons.h"
#include "text_metrics.h"
#include "gfx_metrics.h"
// #include "fonts_externs.h"
#include "fonts_data.h"
#include "Fonts/FreeSansBold48pt7b.h"
#include "Fonts/FreeSansBold36pt7b.h"

// Base icon box size for TL anchoring
const int ICON_BOX_BASE = 32;

// Make sure gfxfont.h is visible via Seeed_GFX/TFT_eSPI includes.
// Do NOT re-typedef GFXglyph/GFXfont.

// Select a FreeFont AND return its pointer (use this instead of selectFreeFont)
static const GFXfont *selectFreeFontPtr(int pt)
{
  switch (pt)
  {
  case 8:
    epaper.setFreeFont(&FreeSansBold8pt7b);
    return &FreeSansBold8pt7b;
  case 9:
    epaper.setFreeFont(&FreeSansBold9pt7b);
    return &FreeSansBold9pt7b;
  case 12:
    epaper.setFreeFont(&FreeSansBold12pt7b);
    return &FreeSansBold12pt7b;
  case 24:
    epaper.setFreeFont(&FreeSansBold24pt7b);
    return &FreeSansBold24pt7b;
  case 36:
    epaper.setFreeFont(&FreeSansBold36pt7b);
    return &FreeSansBold36pt7b;
  case 48:
    epaper.setFreeFont(&FreeSansBold48pt7b);
    return &FreeSansBold48pt7b;
  default:
    epaper.setFreeFont(&FreeSansBold12pt7b);
    return &FreeSansBold12pt7b;
  }
}
//-------------------------------------------------------------------

// Vector primitives
static void drawSun(int x, int y, int s)
{
  int r = 8 * s;
  epaper.fillCircle(x, y, r, TFT_BLACK);
  for (int i = 0; i < 8; i++)
  {
    float a = i * 3.1415926f / 4.0f;
    int x1 = x + (int)((r + 4 * s) * cos(a));
    int y1 = y + (int)((r + 4 * s) * sin(a));
    int x2 = x + (int)((r + 10 * s) * cos(a));
    int y2 = y + (int)((r + 10 * s) * sin(a));
    epaper.drawLine(x1, y1, x2, y2, TFT_BLACK);
  }
}
static void drawCloud(int x, int y, int s, bool fill = true)
{
  int r = 7 * s;
  if (fill)
  {
    epaper.fillCircle(x - 6 * s, y, r, TFT_BLACK);
    epaper.fillCircle(x, y - 3 * s, r + 2 * s, TFT_BLACK);
    epaper.fillCircle(x + 8 * s, y, r + 1 * s, TFT_BLACK);
    epaper.fillRoundRect(x - 12 * s, y, 24 * s, r + 2 * s, 4 * s, TFT_BLACK);
  }
  else
  {
    epaper.drawCircle(x - 6 * s, y, r, TFT_BLACK);
    epaper.drawCircle(x, y - 3 * s, r + 2 * s, TFT_BLACK);
    epaper.drawCircle(x + 8 * s, y, r + 1 * s, TFT_BLACK);
    epaper.drawRoundRect(x - 12 * s, y, 24 * s, r + 2 * s, 4 * s, TFT_BLACK);
  }
}
static void drawRain(int x, int y, int s)
{
  drawCloud(x, y, s, true);
  for (int i = -8; i <= 8; i += 8)
    epaper.drawLine(x + i, y + 10 * s, x + i - 2 * s, y + 14 * s, TFT_BLACK);
}
static void drawSnow(int x, int y, int s)
{
  drawCloud(x, y, s, true);
  for (int i = -6; i <= 6; i += 6)
  {
    epaper.drawLine(x + i, y + 10 * s, x + i, y + 14 * s, TFT_BLACK);
    epaper.drawLine(x + i - 2 * s, y + 12 * s, x + i + 2 * s, y + 12 * s, TFT_BLACK);
  }
}
static void drawStorm(int x, int y, int s)
{
  drawCloud(x, y, s, true);
  epaper.fillTriangle(x, y + 10 * s, x - 3 * s, y + 18 * s, x + 2 * s, y + 18 * s, TFT_BLACK);
}
static void drawFog(int x, int y, int s)
{
  drawCloud(x, y, s, true);
  for (int k = 0; k < 3; k++)
    epaper.drawFastHLine(x - 16 * s, y + 10 * s + 4 * k * s, 32 * s, TFT_BLACK);
}
static void drawWind(int x, int y, int s)
{
  for (int k = 0; k < 3; k++)
  {
    epaper.drawFastHLine(x - 16 * s, y - 4 * s + 6 * k * s, 28 * s, TFT_BLACK);
    epaper.drawCircle(x + 14 * s, y - 4 * s + 6 * k * s, 2 * s, TFT_BLACK);
  }
}
static void drawPartly(int x, int y, int s)
{
  drawCloud(x + 6 * s, y + 2 * s, s, true);
  drawSun(x - 6 * s, y - 6 * s, s);
}

IconType iconForWMO(int code, bool isDay)
{
  if (code == 0)
    return SUN;
  if (code == 1 || code == 2)
    return PARTLY;
  if (code == 3)
    return CLOUD;
  if (code == 45 || code == 48)
    return FOG;
  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82))
    return RAIN;
  if ((code >= 71 && code <= 77) || code == 85 || code == 86)
    return SNOW;
  if (code >= 95)
    return STORM;
  return isDay ? PARTLY : CLOUD;
}

void drawWeatherIcon(IconType t, int cx, int cy, int s)
{
  switch (t)
  {
  case SUN:
    drawSun(cx, cy, s);
    break;
  case PARTLY:
    drawPartly(cx, cy, s);
    break;
  case CLOUD:
    drawCloud(cx, cy, s, true);
    break;
  case RAIN:
    drawRain(cx, cy, s);
    break;
  case STORM:
    drawStorm(cx, cy, s);
    break;
  case SNOW:
    drawSnow(cx, cy, s);
    break;
  case FOG:
    drawFog(cx, cy, s);
    break;
  case WINDY:
    drawWind(cx, cy, s);
    break;
  }
}

void drawWeatherIconTL(IconType t, int left, int top, int s)
{
  const int cx = left + (ICON_BOX_BASE * s) / 2;
  const int cy = top + (ICON_BOX_BASE * s) / 2 + 20;
  drawWeatherIcon(t, cx, cy, s);
}

// Sunrise/Sunset (y is horizon baseline). s = base scale, m = multiplier.
void drawSunriseIcon(int x, int y, int s, int m)
{
  // U = unit size, r = sun radius, s = scale, m = multiplier
  const int U = s * m, r = 4 * U;
  // Draw the sun
  epaper.drawCircle(x, y, r, TFT_BLACK);
  // Erase the bottom half
  epaper.fillRect(x - r, y, 2 * r + 1, r + 2, TFT_WHITE);
  // Draw the horizon line and rays
  epaper.drawFastHLine(x - 6 * U, y, 12 * U, TFT_BLACK);

  int y1 = y - r - 2 * U;
  int y2 = y - r - 4 * U - 5;
  // Draw the sun going up arrow line
  epaper.drawLine(x, y1, x, y2, TFT_BLACK);
  // Draw the right angled line for the arrow effect
  epaper.drawLine(x, y2, x + 3, y1 - 5, TFT_BLACK);
  // Draw the left angled line for the arrow effect
  epaper.drawLine(x, y2, x - 3, y1 - 5, TFT_BLACK);
  // Draw the sun rays
  epaper.drawLine(x - 3 * U, y - r - 1 * U, x - 5 * U, y - r - 3 * U, TFT_BLACK);
  // Draw the sun rays
  epaper.drawLine(x + 3 * U, y - r - 1 * U, x + 5 * U, y - r - 3 * U, TFT_BLACK);
}

void drawSunsetIcon(int x, int y, int s, int m)
{
  const int U = s * m, r = 4 * U;
  int y1 = y - r - 2 * U;
  int y2 = y - r - 4 * U - 3;
  // Draw the sun
  epaper.drawCircle(x, y, r, TFT_BLACK);
  // Erase the bottom half
  epaper.fillRect(x - r, y, 2 * r + 1, r + 2, TFT_WHITE);
  // Draw the horizon line and rays
  epaper.drawFastHLine(x - 6 * U, y, 12 * U, TFT_BLACK);
  // Draw the sun rays
  epaper.drawLine(x, y1, x, y2-2, TFT_BLACK);
  // epaper.drawLine(x, y - r - 2 * U, x, y - r - 4 * U, TFT_BLACK);
  // Draw the right angled line for the arrow effect
  epaper.drawLine(x, y - r - 2 * U, x + 3, y - r - 2 * U - 3, TFT_BLACK);
  // Draw the left angled line for the arrow effect
  epaper.drawLine(x, y - r - 2 * U, x - 3, y - r - 2 * U - 3, TFT_BLACK);
  // Draw the sun rays
  epaper.drawLine(x - 3 * U, y - r - 1 * U, x - 5 * U, y - r - 3 * U, TFT_BLACK);
  // Draw the sun rays
  epaper.drawLine(x + 3 * U, y - r - 1 * U, x + 5 * U, y - r - 3 * U, TFT_BLACK);
}

void drawSunsetIconBelowHorizon(int x, int y, int s, int m)
{
  const int U = s * m, r = 4 * U;
  epaper.drawFastHLine(x - 6 * U, y, 12 * U, TFT_BLACK);
  epaper.drawCircle(x, y, r, TFT_BLACK);
  epaper.fillRect(x - r, y - r - 2, 2 * r + 1, r + 2, TFT_WHITE);
  epaper.drawLine(x, y + r + 2 * U, x, y + r + 4 * U, TFT_BLACK);
  epaper.drawLine(x - 3 * U, y + r + 1 * U, x - 5 * U, y + r + 3 * U, TFT_BLACK);
  epaper.drawLine(x + 3 * U, y + r + 1 * U, x + 5 * U, y + r + 3 * U, TFT_BLACK);
}

// Draw "NN°C" using FreeFonts (TFT_eSPI / Seeed_GFX compatible)
// Assumes you already selected the FreeFont via setFreeFont(...)
// and (recommended) set baseline datum:
///   epaper.setTextDatum(BL_DATUM);

// Call once in your setup() or before drawing:
// epaper.setTextDatum(BL_DATUM);   // baseline-left is best for FreeFonts

// Select FreeFont by "textPoint" (your existing convention)
static void selectFreeFont(int textPoint)
{
  switch (textPoint)
  {
  case 8:
    epaper.setFreeFont(&FreeSansBold8pt7b);
    break;
  case 9:
    epaper.setFreeFont(&FreeSansBold9pt7b);
    break;
  case 12:
    epaper.setFreeFont(&FreeSansBold12pt7b);
    break;
  case 24:
    epaper.setFreeFont(&FreeSansBold24pt7b);
    break;
  // If you don't actually have these headers, do NOT include them:
  case 36:
    epaper.setFreeFont(&FreeSansBold36pt7b);
    break;
  case 48:
    epaper.setFreeFont(&FreeSansBold48pt7b);
    break;
  default:
    epaper.setFreeFont(&FreeSans12pt7b);
    break;
  }
  epaper.setTextSize(1); // don't scale FreeFonts
}

// Return current font height (try EPaper methods, else your helpers)
static int currentFontHeight(int textPoint)
{
#if defined(TFT_eSPI_VERSION) || defined(SEEED_GFX_H) || defined(ARDUINO_TFT_ESPI)
  if constexpr (true)
  { // compile-time branch collapses away
    // Many Seeed_GFX/TFT_eSPI builds expose fontHeight()
    // If your EPaper lacks fontHeight(), comment the next line and use fallback below.
    return epaper.fontHeight();
  }
#endif
  // Fallback to your own helper if exposed:
  return glyphHeight(textPoint); // <-- your existing function
}

// Return width of a string in current font (EPaper method or your helper)
static int currentTextWidth(const String &s, int textPoint)
{
#if defined(TFT_eSPI_VERSION) || defined(SEEED_GFX_H) || defined(ARDUINO_TFT_ESPI)
  // Most builds provide textWidth()
  return epaper.textWidth(s);
#endif
  // Fallback to your own width helper that knows your FreeFont sizing
  return textWidth(s.c_str(), textPoint); // <-- your existing overload
}

// Baseline-left datum recommended:
//   epaper.setTextDatum(BL_DATUM);
//   epaper.setTextSize(1);  // never scale FreeFonts

void drawTempDegC(int x, int y, float t, int /*textSize*/, int textPoint)
{
  const GFXfont *f = selectFreeFontPtr(textPoint);
  char buf[8];
  if (isnan(t))
    strcpy(buf, "--");
  else
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(t));

  const TextMetrics nm = measureWithFont(buf, f);
  const TextMetrics cm = measureWithFont("C", f);

  const int pad = max(2, nm.ascent / 8);
  const int r = max(2, nm.ascent / 6);

  epaper.drawString(buf, x, y);

  // degree circle centered near cap-height: center.y = baseline - (ascent - r)
  const int cx = x + nm.w + pad + r;
  const int cy = y - (nm.ascent - r);
  epaper.drawCircle(cx, cy, r, TFT_BLACK);
  epaper.drawCircle(cx, cy, r - 1, TFT_BLACK);
  epaper.drawCircle(cx, cy, r - 2, TFT_BLACK);

  epaper.drawString("C", cx + r + pad, y);
}

int tempDegCWidth(float t, int textPoint)
{
  const GFXfont *f = selectFreeFontPtr(textPoint);
  char buf[8];
  if (isnan(t))
    strcpy(buf, "--");
  else
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(t));
  const TextMetrics nm = measureWithFont(buf, f);
  const TextMetrics cm = measureWithFont("C", f);
  const int pad = max(2, nm.ascent / 8);
  const int r = max(2, nm.ascent / 6);
  return nm.w + pad + (2 * r) + pad + cm.w;
}

// Scalable UV icon (vector, no fonts).
//  - cx, cy : center of the sun ring
//  - s      : scale (consistent with other icons; try s=2 for cards)
// The look: thick circular ring + 8 rays; a bold “U” inside the ring;
// and a bold “V” immediately to the right of the ring.

void drawUvIcon(int cx, int cy, int s)
{
  // ---- Ring + rays (match sun thickness/spacing)
  const int r = 8 * s; // base radius (same family as drawSun) :contentReference[oaicite:1]{index=1}
  const int t = 2 * s; // ring thickness

  // Ring (thick outline)
  epaper.fillCircle(cx, cy, r, TFT_BLACK);
  epaper.fillCircle(cx, cy, r - t, TFT_WHITE);

  // Rays (8 spokes)
  for (int i = 0; i < 8; ++i)
  {
    // Angle in radians
    float a = i * 3.1415926f / 4.0f;
    int x1 = cx + (int)((r + 2 * s) * cosf(a));
    int y1 = cy + (int)((r + 2 * s) * sinf(a));
    int x2 = cx + (int)((r + 6 * s) * cosf(a));
    int y2 = cy + (int)((r + 6 * s) * sinf(a));
    epaper.drawLine(x1, y1, x2, y2, TFT_BLACK);
  }
  // calculate font height for sizing U/V
  const GFXfont *f = selectFreeFontPtr(9);
  // epaper.setFreeFont(&FreeSansBold9pt7b);
  const TextMetrics fm = measureWithFont("M", f);
  const int fh = fm.h;                                        // font height
  const int fw = fm.w;                                        // font width (of "M")
  epaper.fillRect(cx - s, cy - 6, 4 * fw, fh + 2, TFT_WHITE); // U left stem
  epaper.drawString("  UV", cx - s, cy - 4 * s + 2);
}

// Draw a simple Wi‑Fi indicator anchored at (right, top). The icon is a
// square of size `height` pixels. It draws three concentric arcs (by drawing
// circle outlines and erasing their lower halves) and a filled dot at the
// bottom center. Works with EPaper global.
void drawWifiIcon(int left, int top, int height)
{
  if (height < 16) {
    int cx = left + height / 2;
    int cy = top + height / 2;
    epaper.fillCircle(cx, cy, max(1, height / 6), TFT_BLACK);
    return;
  }

  const int cx = left + height / 2;
  const int baseY = top + height - 2;   // bottom reference

  const int thickness = max(1, height / 10);
  const int gap       = max(3, height / 14);

  const float startAngle = 40.0f;
  const float endAngle   = 140.0f;

  auto drawArcBand = [&](int radius)
  {
    int innerR = radius - thickness;

    for (float a = startAngle; a <= endAngle; a += 2.0f)
    {
      float rad = a * DEG_TO_RAD;

      int xOuter = cx + (int)(cos(rad) * radius);
      int yOuter = baseY - (int)(sin(rad) * radius);

      int xInner = cx + (int)(cos(rad) * innerR);
      int yInner = baseY - (int)(sin(rad) * innerR);

      epaper.drawLine(xInner, yInner, xOuter, yOuter, TFT_BLACK);
    }
  };

  // 4 arcs (largest → smallest)
  int r1 = height / 2;
  int r2 = r1 - thickness - gap;
  int r3 = r2 - thickness - gap;
  int r4 = r3 - thickness - gap;

  drawArcBand(r1);
  drawArcBand(r2);
  drawArcBand(r3);
  drawArcBand(r4);

  // Dot
  int dotR = max(3, thickness - 1);
  epaper.fillCircle(cx, baseY - dotR, dotR, TFT_BLACK);
}


// void drawUvIcon1(int x, int y, int s)
// {
//   const int U = s;
//   epaper.drawRoundRect(x, y, 14, 34, 6, TFT_BLACK);
//   epaper.fillCircle(x + 7, y + 28, 6, TFT_BLACK);
//   epaper.drawFastVLine(x + 7, y + 6, 18, TFT_BLACK);
// }
