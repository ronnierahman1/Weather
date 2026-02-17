#pragma once
#include <Arduino.h>

// --- Prayers (used by weather_api.cpp optional section) ---
struct PrayerDay {
  int fajr = -1, dhuhr = -1, asr = -1, maghrib = -1, isha = -1;  // minutes from 00:00, -1=N/A
  int fajrJ = -1, dhuhrJ = -1, asrJ = -1, maghribJ = -1, ishaJ = -1;
  String hijriPretty;
};

// --- Mini day summary used by the dashboard cards ---
struct MiniDay {
  int low = 0;
  int high = 0;
  int uv = 0;
  String uvStr;
  String phrase;
  int code = 0; // WMO code for icon mapping
  bool isDay = false;
};

// --- Master state for fetching + binding + UI ---
struct WeatherState {
  // RAW "current" (written by fetchCurrentAndHourly)
  float currentTemp   = NAN;
  float currentFeels  = NAN;
  int   currentHum    = -1;
  float currentWind   = NAN;
  float currentPrecip = NAN;
  int   currentCode   = 0;
  bool  currentIsDay         = false;
  // RAW hourly (written by fetchCurrentAndHourly)
  static const int MAX_HOURLY = 48;   // large enough; loop uses hourlyCount
  int    hourlyCount = 0;
  String hourlyTime[MAX_HOURLY];
  float  hourlyTemp[MAX_HOURLY];
  int    hourlyCode[MAX_HOURLY];

  // RAW daily (written by fetchDaily)
  static const int MAX_DAILY = 8;     // keep 7 days (+1 headroom)
  int    dailyCount = 0;
  String dailyDate[MAX_DAILY];
  String dailyDayAbbr[MAX_DAILY];
  int    dailyCode[MAX_DAILY];
  float  dailyMax[MAX_DAILY];
  float  dailyMin[MAX_DAILY];
  int    dailyPrecipProb[MAX_DAILY];
  float  dailyWindMax[MAX_DAILY];
  String dailySunrise[MAX_DAILY];
  String dailySunset[MAX_DAILY];
  int    dailyUVMax[MAX_DAILY];

  // UI-FACING summary (read by ui_weather.cpp; filled by binder)
  int    temp = 0;            // big number center
  int    feelsLike = 0;       // right column
  int    humidity = 0;        // right column
  String condition;           // "Sunny", "Light Rain", ...
  String location;            // footer right
  String updated;             // optional stamp

  MiniDay today;              // left cards + right low/high
  MiniDay tomorrow;

  // Prayers (used if you compile that section in weather_api.cpp)
  PrayerDay todayPray;
  PrayerDay tomorrowPray;
  int lastPrayerFetchYday = -1;
};
