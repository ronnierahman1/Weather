#include <Arduino.h>
#include "config.h"
#include "weather_data.h"

// Simple WMO → phrase mapping (tweak text if you prefer)
static String wmoToPhrase(int code) {
  switch (code) {
    case 0:  return "Sunny";
    case 1: case 2: return "Partly cloudy";
    case 3:  return "Cloudy";
    case 45: case 48: return "Fog";
    case 51: case 53: case 55: return "Drizzle";
    case 56: case 57: return "Freezing Drizzle";
    case 61: case 63: case 65: return "Rain";
    case 66: case 67: return "Freezing Rain";
    case 71: case 73: case 75: return "Snow";
    case 77: return "Snow grains";
    case 80: case 81: case 82: return "Rain showers";
    case 85: case 86: return "Snow showers";
    case 95: return "Thunderstorm";
    case 96: case 99: return "Thunder + hail";
    default: return "—";
  }
}


// Returns a verbal UV category using WHO thresholds.
// 0–2  : Low
// 3–5  : Moderate
// 6–7  : High
// 8–10 : Very High
// 11+  : Extreme
String getUvString(float uv) {
  if (isnan(uv) || uv < 0.0f) return "Unknown";
  if (uv < 3.0f)  return "Low";
  if (uv < 6.0f)  return "Moderate";
  if (uv < 8.0f)  return "High";
  if (uv < 11.0f) return "Very High";
  return "Extreme";
}



void bindWeatherForDashboard(WeatherState& S) {
  // Center / right column
  S.temp      = (int)round(S.currentTemp);
  S.feelsLike = (int)round(S.currentFeels);
  S.humidity  = S.currentHum;
  S.condition = wmoToPhrase(S.currentCode);

  // Footer location: prefer friendly name, else lat/lon (if you keep those in config)
  #ifdef WEATHER_LOCATION_NAME
    S.location = WEATHER_LOCATION_NAME;
  #elif defined(PRAYER_CITY)
    S.location = String(PRAYER_CITY);
  #else
    S.location = "—";
  #endif

  // Today (index 0) + Tomorrow (index 1)
  if (S.dailyCount >= 1) {
    S.today.low   = (int)round(S.dailyMin[0]);
    S.today.high  = (int)round(S.dailyMax[0]);
    S.today.uv    = S.dailyUVMax[0];
    S.today.code  = S.dailyCode[0];
    S.today.phrase= wmoToPhrase(S.dailyCode[0]);
    S.today.uvStr = getUvString(S.today.uv);
    S.today.isDay = S.currentIsDay;
  }
  if (S.dailyCount >= 2) {
    S.tomorrow.low    = (int)round(S.dailyMin[1]);
    S.tomorrow.high   = (int)round(S.dailyMax[1]);
    S.tomorrow.uv     = S.dailyUVMax[1];
    S.tomorrow.code   = S.dailyCode[1];
    S.tomorrow.phrase = wmoToPhrase(S.dailyCode[1]);
    S.tomorrow.isDay  = true; // always show day icon for tomorrow
    S.tomorrow.uvStr  = getUvString(S.tomorrow.uv);
  }

  // Optional status
  S.updated = "Live";
}
