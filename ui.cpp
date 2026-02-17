#include "esp32-hal.h"
#include <time.h>
#include "ui.h"
#include "config.h"
#include "text_metrics.h"
#include "icons.h"
// #include "fonts_externs.h"
#include "gfx_metrics.h"
#include "fonts_data.h"
#include "Fonts/FreeSansBold8pt7b.h"
// Canvas geometry
static constexpr int WIDTH = 800, HEIGHT = 480;
static constexpr int HEADER_H = 40;

static constexpr int NOW_TOP = HEADER_H;
static constexpr int NOW_H   = 200;
static constexpr int NOW_LEFT_W = 400;

static constexpr int HOURLY_Y = NOW_TOP + NOW_H;
static constexpr int HOURLY_H = 100;

static constexpr int DAILY_Y  = HOURLY_Y + HOURLY_H;
static constexpr int DAILY_H  = HEIGHT - DAILY_Y;
String getNextSalahRemainingTimeText(const WeatherState& S, tm ti);

String CurrentLocalTime()
{
  struct tm ti;
  if (!getLocalTime(&ti)) return String("--:--");
  char buf[32];
  strftime(buf, sizeof(buf), "%I:%M %p", &ti);
  return String(buf);
}

// Header
static void printHeaderTextBar(EPaper& epaper, const String& lastUpdated) {
  epaper.fillRect(0,0,WIDTH,HEADER_H,TFT_WHITE);
  epaper.drawFastHLine(0,HEADER_H,WIDTH,TFT_BLACK);
  epaper.setTextSize(1);
  epaper.setFreeFont(&FreeSans12pt7b);

  String line = "XIAO ePaper | Weather: Open-Meteo | Last updated: " + lastUpdated;
  epaper.drawString(line, 10, 10);
}

// ───────────────────────── Clock box (top-right) ─────────────────────────
void drawClockBox(EPaper& epaper, const WeatherState& S, bool clearFirst = false) {
    const int WIDTH  = 800, HEIGHT = 480;
    const int HEADER_H = 40;
    const int NOW_TOP = HEADER_H, // 40
              NOW_H = 200, 
              NOW_LEFT_W = 400;

    const int CLOCK_X = NOW_LEFT_W, // 400
              CLOCK_Y = NOW_TOP + 10; // 50
    const int CLOCK_W = WIDTH - CLOCK_X - 10, // 390
              CLOCK_H = NOW_H - 20; // 180
    const int main_clock_offset = 5, date_Y_offset = 45;
    // clear the box
    if (clearFirst)
    { 
      epaper.fillRect(CLOCK_X + 5, CLOCK_Y, (CLOCK_W/2), 50, TFT_BLACK); // (405, 50, 400, 180)
      epaper.fillRect(CLOCK_X + 5, CLOCK_Y, (CLOCK_W/2), 50, TFT_WHITE); // (405, 50, 400, 180)
      epaper.update();
    }
    
    // time/date
    struct tm ti; if (!getLocalTime(&ti)) return;
    char timeStr[16];  strftime(timeStr, sizeof(timeStr), "%H:%M", &ti);
    char dateStr[32];  strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y", &ti);

    epaper.setTextSize(1);
    epaper.setFreeFont(&FreeSansBold24pt7b);                           // was setTextSize(2)

    int tx = CLOCK_X + main_clock_offset, ty = CLOCK_Y + main_clock_offset;
    // epaper.drawString("            ", tx, ty);
    // epaper.update(tx, ty, CLOCK_W, 40,0); // clear previous time
    epaper.drawString(timeStr, tx, ty);

    epaper.setFreeFont(&FreeSansBold9pt7b);                           // was setTextSize(2)
    epaper.drawString(dateStr, tx, ty + date_Y_offset-6);

    // Hijri date (today)
    if (S.todayPray.hijriPretty.length()) {
        epaper.setFreeFont(&FreeSansBold8pt7b);
        epaper.drawString("Hijri: " + S.todayPray.hijriPretty, tx, ty + date_Y_offset + 12);
    }
    // Table of prayers (today)
    auto mm2str = [](int m)->String {
        if (m<0) return String("--:--");
        int H = (m/60)%24, M = m%60; char b[6]; snprintf(b,sizeof(b),"%02d:%02d",H,M); return String(b);
    };

    const int rowY0   = ty + date_Y_offset + 16 + 12;   // start below Hijri
    const int rowH    = 15;
    const int col1X   = tx;                  // label
    const int col2X   = tx + 120;            // time

    epaper.setFreeFont(&FreeSansBold8pt7b);
    epaper.drawString("Prayers (today)", col1X, rowY0);
    int y = rowY0 + rowH;
    
    epaper.setFreeFont(&FreeSansBold8pt7b);
    epaper.drawString("                                        ", col1X, y+ 5*rowH );  // show under the table
    epaper.update(); // clear previous line

    epaper.drawString("Fajr",    col1X, y); epaper.drawString(mm2str(S.todayPray.fajr),    col2X, y); y+=rowH;
    epaper.drawString("Dhuhr",   col1X, y); epaper.drawString(mm2str(S.todayPray.dhuhr),   col2X, y); y+=rowH;
    epaper.drawString("Asr",     col1X, y); epaper.drawString(mm2str(S.todayPray.asr),     col2X, y); y+=rowH;
    epaper.drawString("Maghrib", col1X, y); epaper.drawString(mm2str(S.todayPray.maghrib), col2X, y); y+=rowH;
    epaper.drawString("Isha",    col1X, y); epaper.drawString(mm2str(S.todayPray.isha),   col2X, y); y+=rowH;

    // Countdown: next event (Adhan/Jama'a), updates every minute (this function is called every minute)
    String line = getNextSalahRemainingTimeText(S, ti);

    epaper.drawString(line, col1X, y );  // show under the table
}

String getNextSalahRemainingTimeText(const WeatherState& S, tm ti)
{
    auto nextLabelAndDelta = [&](const PrayerDay& d, int nowMin)->std::pair<String,int> {
        struct Item { const char* label; int at; const char* altLabel; int altAt; };
        // For each prayer: Adhan then Jama'a
        Item seq[] = {
        {"Adhan Fajr",    d.fajr,    "Jama'a Fajr",    d.fajrJ},
        {"Adhan Dhuhr",   d.dhuhr,   "Jama'a Dhuhr",   d.dhuhrJ},
        {"Adhan Asr",     d.asr,     "Jama'a Asr",     d.asrJ},
        {"Adhan Maghrib", d.maghrib, "Jama'a Maghrib", d.maghribJ},
        {"Adhan Isha",    d.isha,    "Jama'a Isha",    d.ishaJ},
        };
        // walk today
        for (auto &it : seq) {
        if (it.at >= 0 && nowMin < it.at) return {String(it.label), it.at - nowMin};
        if (it.at >= 0 && it.altAt >= 0 && nowMin >= it.at && nowMin < it.altAt)
            return {String(it.altLabel), it.altAt - nowMin};
        }
        // after Isha Jama'a → next Adhan is tomorrow Fajr
        if (S.tomorrowPray.fajr >= 0) {
        int tillMid = 24*60 - nowMin;
        return {String("Adhan Fajr"), tillMid + S.tomorrowPray.fajr};
        }
        return {String("Adhan Fajr"), -1};
    };

    int nowMin = ti.tm_hour*60 + ti.tm_min;
    auto nd = nextLabelAndDelta(S.todayPray, nowMin);
    auto fmtDelta = [](int dm)->String{
        if (dm < 0) return String("--");
        int h = dm/60, m = dm%60;
        if (h>0) { char b[32]; snprintf(b,sizeof(b),"%dh %dm",h,m); return String(b); }
        char b[16]; snprintf(b,sizeof(b),"%dm",m); return String(b);
    };

    String line = "Next: " + nd.first + " in " + fmtDelta(nd.second);
    return line;
}

// HH label from ISO time "YYYY-MM-DDTHH:MM"
static String hhLabel(const String& iso) {
  int i = iso.indexOf('T'); return (i<0) ? iso : iso.substring(i+1,i+3);
}


// Big numeric temperature with a small degree circle (same placement)
static void drawTempBig(EPaper& d, int cx, int baseY, int value, const GFXfont* f,bool isBig=true) {
  char num[8];
  snprintf(num, sizeof(num), "%d", value);

  d.setFreeFont(f);
  const int w = d.textWidth(num);
  const int x = cx - w/2;
  if(isBig)
  {
    d.drawString(num, x, baseY);
  }
  else
  {
    d.drawString(num, x, baseY);
  }

  // degree-circle offset tuned for FreeSansBold48
  int dotX = x + w + 10;
  int dotY = baseY;
  int radius = 0;
  if(isBig)
  {
    dotX += 5;
    dotY += 5;
    radius = 6;
  }
  else
  {
    dotY += 2;
    radius = 3;
  }
  
  d.drawCircle(dotX, dotY, radius, TFT_BLACK);
  d.drawCircle(dotX, dotY, radius - 1, TFT_BLACK);
  if(isBig)
  {
    d.drawCircle(dotX, dotY, radius - 2, TFT_BLACK);
  }

  d.drawString("C", dotX + radius + (isBig?5:2), baseY);
}


// Left panel: big temp, details, right-aligned weather icon, sunrise/sunset
static void drawNowLeft(EPaper& epaper, const WeatherState& S) {
  epaper.drawFastVLine(NOW_LEFT_W, NOW_TOP, NOW_H, TFT_BLACK);

  const int MARGIN_L    = 20;
  const int TOP_PAD     = 10;
  const int TEMP_SIZE   = 5;
  const int DETAIL_SIZE = 2;
  const int LINE_H      = glyphHeight(DETAIL_SIZE);
  const int V_GAP       = LINE_H / 3;
  const int ICON_SCALE  = 3;

  // Big temp
  const int tempX = MARGIN_L;
  const int tempY = NOW_TOP + TOP_PAD;
  // drawTempDegC(tempX, tempY, S.currentTemp, 1, 36);
  drawTempBig(epaper, tempX + 40, tempY, S.currentTemp, &FreeSansBold48pt7b, true);

  // Weather icon, right-aligned in left pane
  const int numH     = glyphHeight(TEMP_SIZE);
  const int iconBox  = ICON_BOX_BASE * ICON_SCALE;
  int iconTop        = tempY + (numH - iconBox) / 2 + 10;
  const int iconLeft = NOW_LEFT_W - MARGIN_L - iconBox;
  drawWeatherIconTL(iconForWMO(S.currentCode, true), iconLeft, iconTop+15, ICON_SCALE);

  // Detail lines
  // epaper.setTextSize(DETAIL_SIZE);
  epaper.setTextSize(1);
  epaper.setFreeFont(&FreeSansBold9pt7b);
  int y = tempY + numH + V_GAP+30;

  epaper.drawString("Feels like:", MARGIN_L, y);
  const int feelsX = MARGIN_L + textWidth("Feels like:", DETAIL_SIZE);
  // drawTempDegC(feelsX, y, S.currentFeels, 1, 9);
  drawTempBig(epaper, feelsX, y, S.currentFeels, &FreeSansBold9pt7b, false);
  y += LINE_H + V_GAP;

  epaper.setTextSize(1);
  epaper.setFreeFont(&FreeSansBold9pt7b);
  char humBuf[24];  snprintf(humBuf, sizeof(humBuf), "Hum %d%%", S.currentHum);
  epaper.drawString(humBuf, MARGIN_L, y);  y += LINE_H + V_GAP;

  char windBuf[32]; snprintf(windBuf,sizeof(windBuf),"Wind %.0f %s", isnan(S.currentWind)?0:S.currentWind, WIND_UNIT);
  epaper.drawString(windBuf, MARGIN_L, y); y += LINE_H + V_GAP;

  char precipBuf[32]; snprintf(precipBuf,sizeof(precipBuf), "Precip %.1f mm", isnan(S.currentPrecip)?0:S.currentPrecip);
  epaper.drawString(precipBuf, MARGIN_L, y);

  // Sunrise/Sunset immediately after precip (at bottom of left pane)
  if (S.dailyCount > 0) {
    String up = S.dailySunrise[0], dn = S.dailySunset[0];
    // Serial.println("1) Sunrise/Sunset: " + up + " / " + dn);
    up = (up.length() >= 5) ? up.substring(0,5) : String("--:--");
    dn = (dn.length() >= 5) ? dn.substring(0,5) : String("--:--");
    // Serial.println("2) Sunrise/Sunset: " + up + " / " + dn);

    const int innerW   = NOW_LEFT_W - 2*MARGIN_L;
    const int colW     = innerW / 2;
    const int leftColX = MARGIN_L;
    const int rightColX= MARGIN_L + colW;

    const int BOTTOM_PAD = V_GAP;
    const int srY        = NOW_TOP + NOW_H - BOTTOM_PAD;

    const int sIcon   = 1, m = 2;
    const int U       = sIcon * m;
    const int ICON_W  = 12 * U;                 // matches icon drawing width
    const int ICON_PAD= glyphWidth(DETAIL_SIZE);
    const int textY   = srY - LINE_H;

    epaper.setTextSize(1);
    epaper.setFreeFont(&FreeSansBold9pt7b);

    int iconLeftL   = leftColX + glyphWidth(DETAIL_SIZE)-10;
    int srCenterX   = iconLeftL + (ICON_W / 2);
    drawSunriseIcon(srCenterX, srY, sIcon, m);
    epaper.setFreeFont(&FreeSansBold9pt7b);
    epaper.drawString(up, iconLeftL + ICON_W + ICON_PAD, textY);

    int iconLeftR   = rightColX - 75;// + glyphWidth(DETAIL_SIZE);
    int ssCenterX   = iconLeftR + (ICON_W / 2);
    drawSunsetIcon(ssCenterX, srY, sIcon, m);
    epaper.setFreeFont(&FreeSansBold9pt7b);
    epaper.drawString(dn, iconLeftR + ICON_W + ICON_PAD, textY);
  }
}

static void drawHourly(EPaper& epaper, const WeatherState& S) {
  epaper.drawFastHLine(0, HOURLY_Y, WIDTH, TFT_BLACK);
  epaper.setTextSize(1);
  epaper.setFreeFont(&FreeSansBold9pt7b);
  // epaper.drawString("Next hours", 10, HOURLY_Y + 10);

  int cellW = WIDTH / max(1, HOURLY_SHOW);
  int y0 = HOURLY_Y + 10;
  
  for (int i=0;i<S.hourlyCount;i++){
    int cx = i*cellW + cellW/2;
    epaper.setFreeFont(&FreeSansBold8pt7b);
    epaper.drawString(hhLabel(S.hourlyTime[i])+"h", cx-10, y0-0);
    drawWeatherIcon(iconForWMO(S.hourlyCode[i], true), cx, y0+35, 1);
    // char tbuf[12]; snprintf(tbuf,sizeof tbuf,"%.0f", S.hourlyTemp[i]);
    // drawTempDegC(cx-8, y0+42, S.hourlyTemp[i], 1, 9);
    drawTempBig(epaper, cx-8, y0+62, S.hourlyTemp[i], &FreeSansBold8pt7b, false);
    // epaper.drawString(tbuf, cx-8, y0+42);
  }
}

static void drawDaily(EPaper& epaper, const WeatherState& S) {
  epaper.setTextSize(1);
  epaper.drawFastHLine(0, DAILY_Y, WIDTH, TFT_BLACK);
  epaper.setFreeFont(&FreeSansBold9pt7b);
  // epaper.drawString("7-day", 10, DAILY_Y + 10);
  
  if (S.dailyCount <= 0) return;

  int cellW = WIDTH / S.dailyCount;
  int y0 = DAILY_Y + 5;

  for (int i=0;i<S.dailyCount;i++){
    int x = i*cellW, xText = x + 6, xMid = x + cellW/2;
    epaper.drawFastVLine(x, DAILY_Y, DAILY_H, TFT_BLACK);

    // epaper.setTextSize(1);
    String mmdd = S.dailyDate[i].length() >= 5 ? S.dailyDate[i].substring(5) : S.dailyDate[i];
    epaper.setFreeFont(&FreeSansBold8pt7b);
    epaper.drawString(S.dailyDayAbbr[i] + " " + mmdd, xText, y0);

    drawWeatherIcon(iconForWMO(S.dailyCode[i], true), xMid, y0+30, 1);

    char tbuf[24]; snprintf(tbuf,sizeof tbuf,"Max: ", S.dailyMax[i]);
    epaper.setFreeFont(&FreeSansBold8pt7b);
    epaper.drawString(tbuf, xText, y0 + 50);
    // drawTempDegC(xText + 35, y0 + 50, S.dailyMax[i], 1, 8);
    drawTempBig(epaper, xText + 45, y0 + 50, S.dailyMax[i], &FreeSansBold8pt7b, false);

    char tbuf1[24]; snprintf(tbuf1,sizeof tbuf1,"Min: ", S.dailyMin[i]);
    epaper.setFreeFont(&FreeSansBold8pt7b);
    epaper.drawString(tbuf1, xText, y0+65);
    // drawTempDegC(xText + 35, y0 + 65, S.dailyMin[i], 1, 8);
    drawTempBig(epaper, xText + 40, y0 + 65, S.dailyMin[i], &FreeSansBold8pt7b, false);

    char pbuf[16]; snprintf(pbuf,sizeof pbuf,"Precip:%d%%", S.dailyPrecipProb[i]);
    epaper.setFreeFont(&FreeSansBold8pt7b);
    epaper.drawString(pbuf, xText, y0 + 80);

    char wbuf[24]; snprintf(wbuf,sizeof wbuf,"Wind:%.0f %s", S.dailyWindMax[i], WIND_UNIT);
    epaper.drawString(wbuf, xText, y0 + 95);

    // small sunrise/sunset in daily grid (compact)
    String up = S.dailySunrise[i], dn = S.dailySunset[i];
    up = (up.length() >= 5) ? up.substring(0,5) : String("--:--");
    dn = (dn.length() >= 5) ? dn.substring(0,5) : String("--:--");
    int srY = y0 + 120;
    drawSunriseIcon(xText + 2, srY, 1, 1);
    epaper.setFreeFont(&FreeSansBold8pt7b);
    epaper.drawString(up, xText + 9, srY - glyphHeight(1));
    drawSunsetIcon (xText + 57, srY, 1, 1);
    epaper.setFreeFont(&FreeSansBold8pt7b); 
    epaper.drawString(dn, xText + 64, srY - glyphHeight(1));
  }
}

void renderAllDeepClean(EPaper& epaper, const WeatherState& S, const String& lastUpdated) {
  deepClean(epaper);
//   epaper.fillScreen(TFT_WHITE);
  printHeaderTextBar(epaper, lastUpdated);
  drawNowLeft(epaper, S);
  drawClockBox(epaper, S, true);
  drawHourly(epaper, S);
  drawDaily(epaper, S);
  epaper.update();
}

void renderAll(EPaper& epaper, const WeatherState& S, const String& lastUpdated) {  
  fullClearOnce(epaper);
  printHeaderTextBar(epaper, lastUpdated);
  drawNowLeft(epaper, S);
  drawClockBox(epaper, S, false);
  drawHourly(epaper, S);
  drawDaily(epaper, S);
  epaper.update();
}
void renderTopHalf(EPaper& epaper, const WeatherState& S, const String& lastUpdated, PageRenderMode mode) {  
  // fullClearOnce(epaper);
  printHeaderTextBar(epaper, lastUpdated);
  if(mode == PageRenderMode::Second_Page)
  {
    drawClockBox(epaper, S, true);
  }
  else
  {
    drawClockBox(epaper, S, false);    
    drawNowLeft(epaper, S);
  }
  // drawClockBox(epaper, S, false);
  epaper.update();
}


void fullClearOnce(EPaper& epaper) { 
  epaper.fillScreen(TFT_WHITE); 
  epaper.update(); 
  }
void deepClean    (EPaper& epaper) { epaper.fillScreen(TFT_BLACK); epaper.update();
                                     epaper.fillScreen(TFT_WHITE); epaper.update();
                                     delay(50); }

