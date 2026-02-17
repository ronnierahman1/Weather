// #pragma once

// // Wi-Fi (fill in)
// #define WIFI_SSID     "Fibre_MarocTelecom-E80B"
// #define WIFI_PASSWORD "evG8SQgbAE"

// // Location (Casablanca default)
// static constexpr float LAT = 33.5731f;
// static constexpr float LON = -7.5898f;

// // Time / NTP (tune DST as needed)
// static constexpr const char* NTP_SERVER      = "pool.ntp.org";
// static constexpr long        GMT_OFFSET_SEC  = 3600;  // UTC+1
// static constexpr int         DAYLIGHT_OFFSET_SEC = 0;

// // Units
// static constexpr const char* WIND_UNIT = "km/h";   // temp '°C' drawn via vector, no UTF-8

// // Behavior
// static constexpr int   HOURLY_SHOW         = 12;              // always show next 12 hours
// static constexpr long  WEATHER_INTERVAL_MS = 15L * 60L * 1000L;


#pragma once
#include <TFT_eSPI.h>   // brings EPaper type + TFT_* colors

// Single shared EPaper instance is defined in Weather.ino
extern EPaper epaper;
