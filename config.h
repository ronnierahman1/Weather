#pragma once

// Wi-Fi credentials — loaded from secrets.h (not committed)
// Copy secrets.h.example to secrets.h and fill in your values.
#include "secrets.h"
// Raspberry Pi API base
#define PI_BASE_URL  "http://192.168.1.1:8088"
#pragma once

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
