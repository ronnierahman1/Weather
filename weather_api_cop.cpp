// #include <Arduino.h>
// #include <WiFiClientSecure.h>
// #include <HTTPClient.h>
// #include <ArduinoJson.h>
// #include <time.h>
// #include "config.h"
// #include "weather_api.h"


// // Simple HTTPS GET (insecure to avoid CA bundle)
// static bool httpGET(const String& url, String& payload) {
//   WiFiClientSecure client; client.setInsecure();
//   HTTPClient http;
//   if (!http.begin(client, url)) return false;
//   int code = http.GET();
//   if (code == HTTP_CODE_OK) { payload = http.getString(); http.end(); return true; }
//   http.end(); return false;
// }

// static String ddmmyyyy(int y, int m, int d) {
//   char b[16]; snprintf(b, sizeof(b), "%02d-%02d-%04d", d, m, y);
//   return String(b);
// }

// bool fetchCurrentAndHourly(WeatherState& S) {
//   // Request TWO days so the 12-hour strip can cross midnight
//   String url = String("https://api.open-meteo.com/v1/forecast?")
//              + "latitude=" + String(LAT,4) + "&longitude=" + String(LON,4)
//              + "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,weather_code,wind_speed_10m"
//              + "&hourly=temperature_2m,weather_code"
//              + "&forecast_days=2&timezone=auto";

//   String body; if (!httpGET(url, body)) return false;

//   DynamicJsonDocument doc(32768);
//   if (deserializeJson(doc, body)) return false;

//   S.currentTemp   = doc["current"]["temperature_2m"] | NAN;
//   S.currentFeels  = doc["current"]["apparent_temperature"] | NAN;
//   S.currentHum    = doc["current"]["relative_humidity_2m"] | -1;
//   S.currentWind   = doc["current"]["wind_speed_10m"] | NAN;
//   S.currentPrecip = doc["current"]["precipitation"] | NAN;
//   S.currentCode   = doc["current"]["weather_code"] | 0;

//   JsonArray tArr    = doc["hourly"]["time"].as<JsonArray>();
//   JsonArray tempArr = doc["hourly"]["temperature_2m"].as<JsonArray>();
//   JsonArray codeArr = doc["hourly"]["weather_code"].as<JsonArray>();
//   const char* curISO = doc["current"]["time"];   // "YYYY-MM-DDTHH:MM"

//   int total = (int)tArr.size();
//   int startIdx = 0;

//   // Find first entry at or after current time
//   if (curISO) {
//     for (int i = 0; i < total; ++i) {
//       const char* ts = tArr[i];
//       if (ts && strcmp(ts, curISO) >= 0) { startIdx = i; break; }
//     }
//   }
//   if (startIdx + HOURLY_SHOW > total) startIdx = max(0, total - HOURLY_SHOW);

//   S.hourlyCount = min(HOURLY_SHOW, total - startIdx);
//   for (int i = 0; i < S.hourlyCount; ++i) {
//     int idx = startIdx + i;
//     S.hourlyTime[i] = (const char*)tArr[idx];
//     S.hourlyTemp[i] = tempArr[idx] | NAN;
//     S.hourlyCode[i] = codeArr[idx] | 0;
//   }
//   return true;
// }

// bool fetchDaily(WeatherState& S) {
//   String url = String("https://api.open-meteo.com/v1/forecast?")
//              + "latitude=" + String(LAT,4) + "&longitude=" + String(LON,4)
//              + "&daily=weather_code,temperature_2m_max,temperature_2m_min,"
//                "precipitation_probability_max,wind_speed_10m_max,sunrise,sunset,uv_index_max"
//              + "&forecast_days=7&timezone=auto";

//   String body; if (!httpGET(url, body)) return false;

//   DynamicJsonDocument doc(24576);
//   if (deserializeJson(doc, body)) return false;

//   JsonArray dateArr    = doc["daily"]["time"].as<JsonArray>();
//   JsonArray wmoArr     = doc["daily"]["weather_code"].as<JsonArray>();
//   JsonArray tmaxArr    = doc["daily"]["temperature_2m_max"].as<JsonArray>();
//   JsonArray tminArr    = doc["daily"]["temperature_2m_min"].as<JsonArray>();
//   JsonArray pprobArr   = doc["daily"]["precipitation_probability_max"].as<JsonArray>();
//   JsonArray windArr    = doc["daily"]["wind_speed_10m_max"].as<JsonArray>();
//   JsonArray sunriseArr = doc["daily"]["sunrise"].as<JsonArray>();
//   JsonArray sunsetArr  = doc["daily"]["sunset"].as<JsonArray>();
//   JsonArray uvArr      = doc["daily"]["uv_index_max"].as<JsonArray>();


//   static const char* DOW[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

//   S.dailyCount = min(7, (int)dateArr.size());
//   for (int i = 0; i < S.dailyCount; i++) {
//     const char* ds = dateArr[i];
//     S.dailyDate[i] = ds ? String(ds) : String("--");

//     const float uvf = uvArr[i] | NAN;
//     S.dailyUVMax[i] = isfinite(uvf) ? (int)roundf(uvf) : 0;

//     // Compute day-of-week abbreviation (local time)
//     int yy=0, mm=0, dd=0;
//     if (ds && sscanf(ds, "%d-%d-%d", &yy, &mm, &dd) == 3) {
//       struct tm tmv = {};
//       tmv.tm_year = yy - 1900; tmv.tm_mon = mm - 1; tmv.tm_mday = dd; tmv.tm_hour = 12;
//       mktime(&tmv);
//       S.dailyDayAbbr[i] = DOW[tmv.tm_wday];
//     } else {
//       S.dailyDayAbbr[i] = "---";
//     }

//     S.dailyCode[i]       = wmoArr[i]   | 0;
//     S.dailyMax[i]        = tmaxArr[i]  | NAN;
//     S.dailyMin[i]        = tminArr[i]  | NAN;
//     S.dailyPrecipProb[i] = pprobArr[i] | 0;
//     S.dailyWindMax[i]    = windArr[i]  | NAN;
//     S.dailySunrise[i]    = (const char*)sunriseArr[i] ? (const char*)sunriseArr[i] : "";
//     S.dailySunset[i]     = (const char*)sunsetArr[i]  ? (const char*)sunsetArr[i]  : "";
//   }
//   return true;
// }

// // ---- helpers ----
// static int hhmmToMin(const String& s) {
//   // accepts "HH:MM", "HH:MM (+01)", or ISO "2025-08-27THH:MM+01:00"
//   int H=-1,M=-1;
//   int tPos = s.indexOf('T');
//   const String& core = (tPos>=0) ? s.substring(tPos+1) : s;
//   int c = core.indexOf(':');
//   if (c>0) { H = core.substring(0,c).toInt(); M = core.substring(c+1,c+3).toInt(); }
//   return (H>=0 && M>=0) ? (H*60+M) : -1;
// }

// static void fillJamaah(PrayerDay& d){
//   // Fajr +20, Dhuhr/Asr/Isha +15, Maghrib +10
//   d.fajrJ    = (d.fajr    <0)?-1 : (d.fajr    + 20);
//   d.dhuhrJ   = (d.dhuhr   <0)?-1 : (d.dhuhr   + 15);
//   d.asrJ     = (d.asr     <0)?-1 : (d.asr     + 15);
//   d.maghribJ = (d.maghrib <0)?-1 : (d.maghrib + 10);
//   d.ishaJ    = (d.isha    <0)?-1 : (d.isha    + 15);
//   // clamp wrap-around to next day (keep minutes possibly > 1440; we’ll mod when needed)
// }

// static String hijriPrettyFromJson(const JsonVariant& hijri){
//   // hijri["day"], hijri["month"]["en"], hijri["year"]
//   String d = hijri["day"] | "";
//   String m = hijri["month"]["en"] | "";
//   String y = hijri["year"] | "";
//   // pad day
//   if (d.length()==1) d = "0"+d;
//   return d + " " + m + " " + y + " AH";
// }

// // Build /v1/timingsByCity/{date}?… per your working request
// static String buildTimingsURL(int year, int month, int day) {
//   String url = "https://api.aladhan.com/v1/timingsByCity/";
//   url += ddmmyyyy(year, month, day);
//   url += "?city=";  url += PRAYER_CITY;
//   url += "&country="; url += PRAYER_COUNTRY;
//   url += "&state="; url += PRAYER_STATE;
//   url += "&method="; url += String(PRAYER_METHOD);
//   url += "&shafaq="; url += PRAYER_SHAFAQ;
//   url += "&tune=";   url += PRAYER_TUNE;
//   url += "&timezonestring="; url += PRAYER_TZ;        // set to "UTC" to mirror your test
//   url += "&calendarMethod="; url += PRAYER_CALMETHOD; // Hijri method
//   return url;
// }


// bool fetchPrayersAndHijri(WeatherState& S) {
//   struct tm ti{}; if (!getLocalTime(&ti)) return false;

//   int y  = ti.tm_year + 1900;
//   int m  = ti.tm_mon + 1;
//   int d  = ti.tm_mday;

//   // ---- today ----
//   String url = buildTimingsURL(y, m, d);
//   String body;
//   if (!httpGET(url, body)) return false;

//   DynamicJsonDocument doc(32768);
//   if (deserializeJson(doc, body)) return false;

//   JsonVariant data = doc["data"];
//   if (data.isNull()) return false;

//   JsonVariant t = data["timings"];
//   S.todayPray.fajr    = hhmmToMin( (const char*) t["Fajr"]    );
//   S.todayPray.dhuhr   = hhmmToMin( (const char*) t["Dhuhr"]   );
//   S.todayPray.asr     = hhmmToMin( (const char*) t["Asr"]     );
//   S.todayPray.maghrib = hhmmToMin( (const char*) t["Maghrib"] );
//   S.todayPray.isha    = hhmmToMin( (const char*) t["Isha"]    );

//   // Jama'a offsets: Fajr +20, Dhuhr/Asr/Isha +15, Maghrib +10
//   S.todayPray.fajrJ    = (S.todayPray.fajr    <0)?-1:(S.todayPray.fajr    + 20);
//   S.todayPray.dhuhrJ   = (S.todayPray.dhuhr   <0)?-1:(S.todayPray.dhuhr   + 15);
//   S.todayPray.asrJ     = (S.todayPray.asr     <0)?-1:(S.todayPray.asr     + 15);
//   S.todayPray.maghribJ = (S.todayPray.maghrib <0)?-1:(S.todayPray.maghrib + 10);
//   S.todayPray.ishaJ    = (S.todayPray.isha    <0)?-1:(S.todayPray.isha    + 15);

//   // Hijri pretty
//   S.todayPray.hijriPretty = hijriPrettyFromJson(data["date"]["hijri"]);

//   // ---- tomorrow (Fajr only, for post-Isha countdown) ----
//   time_t now = time(nullptr);
//   struct tm td{}; localtime_r(&now, &td);
//   td.tm_mday += 1; mktime(&td);           // normalize to tomorrow
//   String url2 = buildTimingsURL(td.tm_year+1900, td.tm_mon+1, td.tm_mday);
//   String body2;
//   if (httpGET(url2, body2)) {
//     DynamicJsonDocument doc2(16384);
//     if (!deserializeJson(doc2, body2)) {
//       JsonVariant d2 = doc2["data"]["timings"];
//       if (!d2.isNull()) {
//         S.tomorrowPray.fajr = hhmmToMin( (const char*) d2["Fajr"] );
//       }
//     }
//   }

//   S.lastPrayerFetchYday = ti.tm_yday;
//   return true;
// }