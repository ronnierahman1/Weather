#include <time.h>
#include "ui.h"
#include "config.h"
#include "text_metrics.h"
#include "icons.h"
// #include "fonts_externs.h"
#include "gfx_metrics.h"
#include "fonts_data.h"

static uint16_t gfxTextWidth(EPaper& d, const String& s) {
  return d.textWidth(s);                 // TFT_eSPI API
}
static uint16_t gfxTextHeight(EPaper& d) {
  return d.fontHeight();                 // height for current font
}
static void drawStringRight(EPaper& d, const String& s, int xRight, int y) {
  d.drawString(s, xRight - (int)d.textWidth(s), y);
}

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

// Case-fold helpers
static inline String lowerCopy(const String& s) {
  String t = s; t.toLowerCase(); return t;
}

static inline String lowerCopy(const char* s) {
  String t = s ? String(s) : String(); t.toLowerCase(); return t;
}



// HH label from ISO time "YYYY-MM-DDTHH:MM"
static String hhLabel(const String& iso) {
  int i = iso.indexOf('T'); return (i<0) ? iso : iso.substring(i+1,i+3);
}

// ───────────────────────── Salah Timetable (full-screen) ─────────────────────────
static constexpr int SCR_W = 800, SCR_H = 480;

static String mmToHHMM12(int m) {
  if (m < 0) return String("--:--");
  int H = (m/60) % 24, M = m % 60;
  int h12 = H % 12; if (h12 == 0) h12 = 12;
  char buf[16]; snprintf(buf, sizeof(buf), "%d:%02d %s", h12, M, (H<12)?"AM":"PM");
  return String(buf);
}
static String isoToHHMM12(const String& iso) {          // "YYYY-MM-DDTHH:MM..."
  // if (iso.length() < 16) return String("--:--");
  int H = iso.substring(11,13).toInt();
  int M = iso.substring(14,16).toInt();
  return mmToHHMM12(H*60 + M);
}

static String toHHMM12(const String& iso) {          // "YYYY-MM-DDTHH:MM..."
  // Serial.println("iso: " + iso);
  // if (iso.length() < 16) return String("--:--");
  int H = iso.substring(0,2).toInt();
  // Serial.println("H: " + String(H));
  int M = iso.substring(3,5).toInt();
  // Serial.println("M: " + String(M));
  return mmToHHMM12(H*60 + M);
}

static int   nowMinutesLocal(){
  struct tm ti{}; if (!getLocalTime(&ti)) return -1;
  return ti.tm_hour*60 + ti.tm_min;
}
static void  dottedH(EPaper& ep, int y, int x0, int x1) {
  for (int x = x0; x < x1; x += 8) ep.drawPixel(x, y, TFT_BLACK);
}
static int   txtW(const String& s, int sz){ return textWidth(s, sz); }
static int   lineH(int sz){ return glyphHeight(sz); }

static String nextPrayerName(const WeatherState& S, int &nextAtMin){
  struct P { const char* name; int t; } p[] = {
    {"FAJR",    S.todayPray.fajr},
    {"DHUHR",   S.todayPray.dhuhr},
    {"ASR",     S.todayPray.asr},
    {"MAGHRIB", S.todayPray.maghrib},
    {"ISHA",    S.todayPray.isha},
  };
  int nowM = nowMinutesLocal();
  if (nowM < 0) { nextAtMin = -1; return String("FAJR"); }
  for (auto &e : p) { if (e.t >= 0 && nowM < e.t) { nextAtMin = e.t; return String(e.name); } }
  // past Isha → tomorrow Fajr (if available)
  nextAtMin = (S.tomorrowPray.fajr>=0) ? (24*60 + S.tomorrowPray.fajr) : -1;
  return String("FAJR");
}

tm nextPrayerTime(const WeatherState& S) {
  int nextAtMin = -1;
  String nextName = nextPrayerName(S, nextAtMin);
  if (nextAtMin < 0) return tm{};
  int nextH = nextAtMin / 60;
  int nextM = nextAtMin % 60;
  struct tm ti{};
  getLocalTime(&ti);
  ti.tm_hour = nextH;
  ti.tm_min = nextM;
  return ti;
}


bool hasNextPrayerTimeElapsed(tm currentTime, tm nextTime) {
  
  if (nextTime.tm_hour == 0 && nextTime.tm_min == 0) return false;  // no valid next prayer time
  return (currentTime.tm_hour * 60 + currentTime.tm_min >= (nextTime.tm_hour * 60 + nextTime.tm_min));
}

void renderSalahTimetable(EPaper& epaper, const WeatherState& S, const String& lastUpdated) {
  epaper.fillRect(0,0,SCR_W,SCR_H,TFT_WHITE);
    // ── Header ──
  struct tm ti{}; getLocalTime(&ti);
  char gDate[64]; strftime(gDate,sizeof(gDate),"%A, %B %d, %Y", &ti);

  const int headerY = 6;
  epaper.setTextSize(1);
  epaper.setFreeFont(&FreeSerifItalic12pt7b);                           // was setTextSize(2)
  epaper.drawString(String(gDate), 10, headerY);

  String hijri = S.todayPray.hijriPretty.length() ? S.todayPray.hijriPretty : String("-- Hijri --");
  int hijW = gfxTextWidth(epaper, hijri);
  epaper.drawString(hijri, SCR_W - 10 - hijW, headerY);

  epaper.drawFastHLine(0, 40, SCR_W, TFT_BLACK);
  epaper.drawFastVLine(400, 40, 90, TFT_BLACK);

  // ── Next section ──
  int nextAt = -1;
  String nextName = nextPrayerName(S, nextAt);
  String nextTime = (nextAt>=0) ? mmToHHMM12(nextAt % (24*60)) : String("--:--");
  String timeHHMM = nextTime.substring(0, nextTime.length()-3);
  String ampm     = nextTime.substring(nextTime.length()-2);

  epaper.setFreeFont(&FreeSans9pt7b);                            // label "Next"
  epaper.drawString("Time Now" , 10, 54);
  String currentTime = CurrentLocalTime();              // HH:MM
  int timeW = gfxTextWidth(epaper, currentTime);
  
  epaper.setFreeFont(&FreeSansBold24pt7b);   
  epaper.drawString(currentTime, 10, 78);

  int timeX = SCR_W /2 +5 ;
  
  epaper.setFreeFont(&FreeSans9pt7b);                            // label "Next"
  epaper.drawString("Next Salah" , timeX, 54);
  epaper.setFreeFont(&FreeSansBold12pt7b);                       // big NEXT PRAYER (left)
  epaper.drawString(nextName + " - " + timeHHMM + " " + ampm, timeX, 78);
  epaper.setTextColor(TFT_WHITE);
  epaper.drawString("                                        ", timeX, 102);
  epaper.setTextColor(TFT_BLACK);
  epaper.drawString(getNextSalahRemainingTimeText(S, ti), timeX, 102);

  epaper.drawFastHLine(0, 130, SCR_W, TFT_BLACK);

  // ── Table header ──
  const int colLx = 10;
  const int colRx = SCR_W - 10;
  int y = 135;

  epaper.setFreeFont(&FreeSansBold12pt7b);                       // "Prayer" / "Time"
  epaper.drawString("Prayer", colLx, y);
  drawStringRight(epaper, "Time", colRx, y);

  y += 38;

  // rows
  struct Row { const char* name; String time; };
  Row rows[] = {
    {"Fajr",    mmToHHMM12(S.todayPray.fajr)},
    {"Sunrise", toHHMM12(S.dailySunrise[0])},
    {"Dhuhr",   mmToHHMM12(S.todayPray.dhuhr)},
    {"Asr",     mmToHHMM12(S.todayPray.asr)},
    {"Sunset",  toHHMM12(S.dailySunset[0])},
    {"Maghrib", mmToHHMM12(S.todayPray.maghrib)},
    {"Isha",    mmToHHMM12(S.todayPray.isha)},
  };


  const int rowGap = 40;
  for (auto &r : rows) {
    dottedH(epaper, y - 10, 0, SCR_W);
  if (lowerCopy(nextName) == lowerCopy(r.name)) {
    epaper.setFreeFont(&FreeSerifBoldItalic12pt7b);
  } else {
    epaper.setFreeFont(&FreeSansBold12pt7b);
  }
    epaper.drawString(r.name, colLx, y);
    drawStringRight(epaper, r.time, colRx, y);
    y += rowGap;
  }
  dottedH(epaper, y - 10, 0, SCR_W);

  // ── Footer ──
  epaper.setFreeFont(&FreeSans9pt7b);
  String method = String("Method: Aladhan (method ") + String(PRAYER_METHOD) + "), shafaq " + String(PRAYER_SHAFAQ);
  epaper.drawString(method, 10, SCR_H - 45);

  String upd   = "Updated: " + (lastUpdated.length() ? lastUpdated : String("--:--"));
  drawStringRight(epaper, upd, SCR_W - 10, SCR_H - 45);

  #ifdef PRAYER_TZ
    drawStringRight(epaper, String(PRAYER_TZ), SCR_W - 10, SCR_H - 25);
  #else
    drawStringRight(epaper, String("Local time"), SCR_W - 10, SCR_H - 24);
  #endif
}
