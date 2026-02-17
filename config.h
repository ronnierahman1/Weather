#pragma once

// Wi-Fi (fill in)
#define WIFI_SSID     "Fibre_MarocTelecom-E80B"
#define WIFI_SSID_5GHZ   "Fibre_MarocTelecom-E80B-5GHz"
#define WIFI_PASSWORD "evG8SQgbAE"
// Raspberry Pi API base
#define PI_BASE_URL  "http://192.168.1.1:8088"
#pragma once


// Location (Casablanca default)
static constexpr float LAT = 33.5731f;
static constexpr float LON = -7.5898f;

// Time / NTP (tune DST as needed)
static constexpr const char* NTP_SERVER      = "pool.ntp.org";
// GMT offset in seconds. Defined in settings.cpp so it can be persisted at runtime.
extern long        GMT_OFFSET_SEC;  // e.g. 0 for UTC, 3600 for UTC+1
static int         DAYLIGHT_OFFSET_SEC = 0;

// Units
static constexpr const char* WIND_UNIT = "km/h";   // temp '°C' drawn via vector, no UTF-8

// Behavior
static constexpr int   HOURLY_SHOW         = 12;              // always show next 12 hours
static constexpr long  WEATHER_INTERVAL_MS = 15L * 60L * 1000L;

// ───────── Aladhan (timingsByCity) — based on your successful call ─────────
// If you prefer local Moroccan times directly, set PRAYER_TZ to "Africa/Casablanca".
// Your test used "UTC" and worked; countdown assumes local times, so "Africa/Casablanca"
// is usually the best choice on-device. Use whichever you prefer.

#define PRAYER_CITY       "Casablanca"
#define PRAYER_COUNTRY    "MA"
#define PRAYER_STATE      "Casablanca"
#define PRAYER_METHOD     21                 // Morocco
#define PRAYER_SHAFAQ     "general"
#define PRAYER_TUNE       "5,0,5,7,-1,-1,0,0,-6" // Imsak,Fajr,Sunrise,Dhuhr,Asr,Maghrib,Sunset,Isha,Midnight
#define PRAYER_TZ         "Africa/Casablanca"   // set to "UTC" to match your screenshot
#define PRAYER_CALMETHOD  "UAQ"                 // Hijri calendar method