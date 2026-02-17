#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <string.h>
#include "config.h"
#include "weather_api.h"
#include <WiFiClient.h>

// --- You already have these somewhere (keep using them) ---
static int  hhmmToMin(const char* s);
static void fillJamaah(PrayerDay& d);

// If you already have an existing httpGET(...) (e.g., TLS/external API), wrap it here.
// Return true on success and put response in payload.
using HttpGetFn = bool (*)(const String& url, String& payload);

// -------------------- Shared low-level GET --------------------
// Plain HTTP GET (LAN / Raspberry Pi)
static bool httpGET_plain(const String& url, String& payload) {
  WiFiClient client;
  HTTPClient http;
  if (!http.begin(client, url)) return false;

  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    payload = http.getString();
    http.end();
    return true;
  }

  http.end();
  return false;
}


// Simple HTTPS GET (insecure to avoid CA bundle)
static bool httpGET(const String& url, String& payload) {
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  if (code == HTTP_CODE_OK) { payload = http.getString(); http.end(); return true; }
  http.end(); return false;
}


// -------------------- Shared parsing/fill logic --------------------
// Normalized fill: expects doc to contain:
// - timings object with Fajr/Dhuhr/Asr/Maghrib/Isha (or your external API equivalents if you map below)
// - hijri_pretty (optional)
// - tomorrow.Fajr (optional)
static bool parseAndFillPrayers(WeatherState& S, const JsonDocument& doc) {

  auto readMin = [&](JsonVariantConst v) -> int {
    if (v.isNull()) return -1;
    if (v.is<int>()) return v.as<int>();                 // Pi format: minutes
    const char* s = v.as<const char*>();                 // External: "HH:MM"
    if (!s) return -1;
    return hhmmToMin(s);
  };

  // --------- 1) Raspberry Pi format ---------
  if (doc.containsKey("today")) {
    JsonObjectConst today = doc["today"].as<JsonObjectConst>();
    if (today.isNull()) return false;

    S.todayPray.fajr    = readMin(today["fajr"]);
    S.todayPray.dhuhr   = readMin(today["dhuhr"]);
    S.todayPray.asr     = readMin(today["asr"]);
    S.todayPray.maghrib = readMin(today["maghrib"]);
    S.todayPray.isha    = readMin(today["isha"]);

    // Optional Jamaah values from Pi (if provided)
    S.todayPray.fajrJ    = readMin(today["fajrJ"]);
    S.todayPray.dhuhrJ   = readMin(today["dhuhrJ"]);
    S.todayPray.asrJ     = readMin(today["asrJ"]);
    S.todayPray.maghribJ = readMin(today["maghribJ"]);
    S.todayPray.ishaJ    = readMin(today["ishaJ"]);

    S.todayPray.hijriPretty = String((const char*)(doc["hijriPretty"] | ""));

    // If Pi already sends J times, don't recompute; otherwise compute here
    if (S.todayPray.fajrJ < 0) fillJamaah(S.todayPray);

    // Tomorrow (Pi usually sends partial; at least fajr)
    S.tomorrowPray = PrayerDay();
    JsonObjectConst tmr = doc["tomorrow"].as<JsonObjectConst>();
    if (!tmr.isNull()) {
      S.tomorrowPray.fajr = readMin(tmr["fajr"]);
      fillJamaah(S.tomorrowPray);
    }
    return true;
  }

  // --------- 2) Existing external format (timings HH:MM strings) ---------
  JsonObjectConst timings = doc["timings"].as<JsonObjectConst>();
  if (timings.isNull()) return false;

  auto getHHMM = [&](const char* key) -> int {
    return readMin(timings[key]);
  };

  S.todayPray.fajr    = getHHMM("Fajr");
  S.todayPray.dhuhr   = getHHMM("Dhuhr");
  S.todayPray.asr     = getHHMM("Asr");
  S.todayPray.maghrib = getHHMM("Maghrib");
  S.todayPray.isha    = getHHMM("Isha");

  const char* hp = doc["hijri_pretty"] | "";
  S.todayPray.hijriPretty = String(hp);

  fillJamaah(S.todayPray);

  S.tomorrowPray = PrayerDay();
  JsonObjectConst tmr = doc["tomorrow"].as<JsonObjectConst>();
  if (!tmr.isNull()) {
    S.tomorrowPray.fajr = readMin(tmr["Fajr"]);
    fillJamaah(S.tomorrowPray);
  }
  return true;
}

// If your *external* API returns a different JSON structure, normalize it here
// into the same fields parseAndFillPrayers expects.
static bool normalizeExternalApiJson(const JsonDocument& in, JsonDocument& out) {
  // TODO: Map your existing external API response into:
  // out["timings"]["Fajr"], ... "Isha"
  // out["tomorrow"]["Fajr"] (optional)
  // out["hijri_pretty"] (optional)
  //
  // If your existing fetchPrayersAndHijri() already has correct parsing,
  // you can move that parsing here and write into `out`.
  return false; // default: implement per your existing API response
}

// -------------------- Shared fetch core --------------------
static bool fetchPrayersCore(WeatherState& S, const String& url, HttpGetFn getFn) {
  time_t now = time(nullptr);
  struct tm ti;
  localtime_r(&now, &ti);
  if (S.lastPrayerFetchYday == ti.tm_yday) return true;

  String body;
  if (!getFn(url, body)) return false;

  DynamicJsonDocument doc(8192);
  if (deserializeJson(doc, body)) return false;

  // If response includes {"ok":...}, enforce it. If not, ignore.
  if (doc.containsKey("ok") && !doc["ok"].as<bool>()) return false;

  if (!parseAndFillPrayers(S, doc)) return false;

  S.lastPrayerFetchYday = ti.tm_yday;
  return true;
}


// -------------------- Public methods --------------------
// External API version (existing behavior) - now uses shared core.
// You only need to implement normalizeExternalApiJson(...) to use shared parsing,
// OR keep your old external parsing and skip normalization (see note below).
bool fetchPrayersAndHijri(WeatherState& S) {
  // your existing external URL builder here
  const String url = /* buildTimingsURL(...) or whatever you already use */ "";
  return fetchPrayersCore(S, url, httpGET);         // uses your HTTPS httpGET()
}

bool fetchPrayersAndHijriFromPi(WeatherState& S) {
  const String url = String(PI_BASE_URL) + "/api/prayers";
  return fetchPrayersCore(S, url, httpGET_plain);   // uses LAN plain http
}

static String ddmmyyyy(int y, int m, int d) {
  char b[16]; snprintf(b, sizeof(b), "%02d-%02d-%04d", d, m, y);
  return String(b);
}

static bool parseCurrentAndHourlyIntoState(WeatherState& S, const JsonDocument& doc) {
  JsonObjectConst cur = doc["current"].as<JsonObjectConst>();
  JsonObjectConst hr  = doc["hourly"].as<JsonObjectConst>();
  if (cur.isNull() || hr.isNull()) return false;

  // ---- Current ----
  S.currentTemp   = cur["temperature_2m"] | NAN;
  S.currentFeels  = cur["apparent_temperature"] | NAN;
  S.currentHum    = cur["relative_humidity_2m"] | -1;
  S.currentWind   = cur["wind_speed_10m"] | NAN;
  S.currentPrecip = cur["precipitation"] | NAN;
  S.currentCode   = cur["weather_code"] | 0;

  // ---- Hourly ----
  JsonArrayConst timeArr    = hr["time"].as<JsonArrayConst>();
  JsonArrayConst tempArr = hr["temperature_2m"].as<JsonArrayConst>();
  JsonArrayConst codeArr = hr["weather_code"].as<JsonArrayConst>();
  const char* curISO = cur["time"] | nullptr;   // "YYYY-MM-DDTHH:MM"

  int total = (int)timeArr.size();
  // Serial.println("Total hourly entries: " + String(total));
  int startIdx = 0;

  // Find first entry at or after current time
  if (curISO) {
    for (int i = 0; i < total; ++i) {
      const char* ts = timeArr[i] | nullptr;

      if (ts && strcmp(ts, curISO) >= 0) {
        startIdx = i;
        break;
      }
    }
  }

  if (startIdx + HOURLY_SHOW > total)
    startIdx = max(0, total - HOURLY_SHOW);

  S.hourlyCount = min(HOURLY_SHOW, total - startIdx);
  for (int i = 0; i < S.hourlyCount; ++i) {
    int idx = startIdx + i;

    const char* ts = timeArr[idx] | "";
    S.hourlyTime[i] = String(ts);     // <-- SAFE

    S.hourlyTemp[i] = tempArr[idx] | NAN;
    S.hourlyCode[i] = codeArr[idx] | 0;
  }


  return true;
}

bool fetchCurrentAndHourlyFromPi(WeatherState& S) {
  String url = String(PI_BASE_URL) + "/api/weather/current_hourly"
             + "?lat=" + String(LAT,4)
             + "&lon=" + String(LON,4);

  String body;
  if (!httpGET_plain(url, body)) return false;

  DynamicJsonDocument doc(32768);
  if (deserializeJson(doc, body)) return false;

  if (doc.containsKey("ok") && !doc["ok"].as<bool>()) return false;

  return parseCurrentAndHourlyIntoState(S, doc);
}

bool fetchCurrentAndHourly(WeatherState& S) {
  String url = String("https://api.open-meteo.com/v1/forecast?")
             + "latitude=" + String(LAT,4)
             + "&longitude=" + String(LON,4)
             + "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,"
               "precipitation,weather_code,wind_speed_10m"
             + "&hourly=temperature_2m,weather_code"
             + "&forecast_days=2&timezone=auto";

  String body;
  if (!httpGET(url, body)) return false;

  DynamicJsonDocument doc(32768);
  if (deserializeJson(doc, body)) return false;

  return parseCurrentAndHourlyIntoState(S, doc);
}

// Shared: parse daily forecast JSON (same structure as open-meteo "daily")
static bool parseDailyIntoState(WeatherState& S, const JsonDocument& doc) {
  JsonObjectConst daily = doc["daily"].as<JsonObjectConst>();
  if (daily.isNull()) return false;

  JsonArrayConst timeArr      = daily["time"].as<JsonArrayConst>();
  JsonArrayConst dowArr       = daily["dow"].as<JsonArrayConst>();
  JsonArrayConst dateLblArr   = daily["date_label"].as<JsonArrayConst>();
  JsonArrayConst sunriseHmArr = daily["sunrise_hm"].as<JsonArrayConst>();
  JsonArrayConst sunsetHmArr  = daily["sunset_hm"].as<JsonArrayConst>();

  JsonArrayConst wmoArr   = daily["weather_code"].as<JsonArrayConst>();
  JsonArrayConst tmaxArr  = daily["temperature_2m_max"].as<JsonArrayConst>();
  JsonArrayConst tminArr  = daily["temperature_2m_min"].as<JsonArrayConst>();
  JsonArrayConst pprobArr = daily["precipitation_probability_max"].as<JsonArrayConst>();
  JsonArrayConst windArr  = daily["wind_speed_10m_max"].as<JsonArrayConst>();
  JsonArrayConst uvArr    = daily["uv_index_max"].as<JsonArrayConst>();

  // Count from date_label (best), else time, else weather_code
  int n = 0;
  if (!dateLblArr.isNull() && dateLblArr.size() > 0) n = (int)dateLblArr.size();
  else if (!timeArr.isNull() && timeArr.size() > 0)  n = (int)timeArr.size();
  else if (!wmoArr.isNull() && wmoArr.size() > 0)    n = (int)wmoArr.size();

  S.dailyCount = min(7, n);
  // Serial.println("Daily count: " + String(S.dailyCount));

  for (int i = 0; i < S.dailyCount; i++) {

    // --- Safe string extraction ---
    const char* dl = nullptr;
    if (!dateLblArr.isNull()) {
      JsonVariantConst v = dateLblArr[i];
      dl = v.isNull() ? nullptr : v.as<const char*>();
    }

    const char* ts = nullptr;
    if (!timeArr.isNull()) {
      JsonVariantConst v = timeArr[i];
      ts = v.isNull() ? nullptr : v.as<const char*>();
    }

    const char* dow = nullptr;
    if (!dowArr.isNull()) {
      JsonVariantConst v = dowArr[i];
      dow = v.isNull() ? nullptr : v.as<const char*>();
    }

    const char* sr = nullptr;
    if (!sunriseHmArr.isNull()) {
      JsonVariantConst v = sunriseHmArr[i];
      sr = v.isNull() ? nullptr : v.as<const char*>();
    }

    const char* ss = nullptr;
    if (!sunsetHmArr.isNull()) {
      JsonVariantConst v = sunsetHmArr[i];
      ss = v.isNull() ? nullptr : v.as<const char*>();
    }

    // Copy into Strings (no pointer lifetime issues)
    S.dailyDate[i] = dl ? String(dl) : (ts ? String(ts) : String("--"));
    S.dailyDayAbbr[i] = dow ? String(dow) : String("---");

    S.dailySunrise[i] = sr ? String(sr) : String("--:--");
    S.dailySunset[i]  = ss ? String(ss) : String("--:--");

    // Numeric fields
    S.dailyCode[i]       = (!wmoArr.isNull())   ? (wmoArr[i]   | 0)   : 0;
    S.dailyMax[i]        = (!tmaxArr.isNull())  ? (tmaxArr[i]  | NAN) : NAN;
    S.dailyMin[i]        = (!tminArr.isNull())  ? (tminArr[i]  | NAN) : NAN;
    S.dailyPrecipProb[i] = (!pprobArr.isNull()) ? (pprobArr[i] | 0)   : 0;
    S.dailyWindMax[i]    = (!windArr.isNull())  ? (windArr[i]  | NAN) : NAN;

    const float uvf = (!uvArr.isNull()) ? (uvArr[i] | NAN) : NAN;
    S.dailyUVMax[i] = isfinite(uvf) ? (int)roundf(uvf) : 0;

    // Serial.println("Daily " + String(i) +
    //                " date=" + S.dailyDate[i] +
    //                " dow=" + S.dailyDayAbbr[i] +
    //                " sr=" + S.dailySunrise[i] +
    //                " ss=" + S.dailySunset[i]);
  }

  return true;
}



bool fetchDaily(WeatherState& S) {
  String url = String("https://api.open-meteo.com/v1/forecast?")
             + "latitude=" + String(LAT,4) + "&longitude=" + String(LON,4)
             + "&daily=weather_code,temperature_2m_max,temperature_2m_min,"
               "precipitation_probability_max,wind_speed_10m_max,sunrise,sunset,uv_index_max"
             + "&forecast_days=7&timezone=auto";

  String body; 
  if (!httpGET(url, body)) return false;

  DynamicJsonDocument doc(24576);
  if (deserializeJson(doc, body)) return false;

  return parseDailyIntoState(S, doc);
}

bool fetchDailyFromPi(WeatherState& S) {
  String url = String(PI_BASE_URL) + "/api/weather/daily"
             + "?lat=" + String(LAT, 4)
             + "&lon=" + String(LON, 4);

  String body;
  if (!httpGET_plain(url, body)) return false;
  // Serial.println("daily pi body: " + String(body));
  // Was 24576 — too small for the wrapped payload
  DynamicJsonDocument doc(40960);

  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    // Serial.print("deserializeJson failed: ");
    // Serial.println(err.c_str());
    return false;
  }

  JsonObjectConst daily = doc["daily"];
  if (daily.isNull()) {
    // Serial.println("daily is NULL. Top-level keys:");
    for (JsonPairConst kv : doc.as<JsonObjectConst>()) {
      // Serial.println(String(kv.key().c_str()));
    }
    return false;
  }

// Dump which fields exist in daily:
// Serial.println("daily keys:");
// for (JsonPairConst kv : daily) {
//   Serial.println(String(kv.key().c_str()));
// }

  if (doc.containsKey("ok") && !doc["ok"].as<bool>()) return false;

  S.dailyCount = 0;                // ensure no stale days remain
  return parseDailyIntoState(S, doc);
}



// ---- helpers ----
// Converts "HH:MM", "HH:MM (+01)", or ISO "YYYY-MM-DDTHH:MM+01:00" to minutes since midnight.
static int hhmmToMin(const char* s) {
  if (!s || !*s) return -1;

  // If ISO timestamp, jump to 'T'
  const char* p = s;
  const char* t = strchr(p, 'T');
  if (t) p = t + 1;

  // Expect HH:MM
  const char* c = strchr(p, ':');
  if (!c || (c - p) < 1) return -1;

  int H = 0, M = 0;

  // parse hour (1–2 digits)
  if (c - p == 1) {
    if (p[0] < '0' || p[0] > '9') return -1;
    H = p[0] - '0';
  } else {
    if (p[0] < '0' || p[0] > '9' || p[1] < '0' || p[1] > '9') return -1;
    H = (p[0] - '0') * 10 + (p[1] - '0');
  }

  // parse minute (2 digits)
  if (!c[1] || !c[2]) return -1;
  if (c[1] < '0' || c[1] > '9' || c[2] < '0' || c[2] > '9') return -1;
  M = (c[1] - '0') * 10 + (c[2] - '0');

  if (H < 0 || H > 23 || M < 0 || M > 59) return -1;
  return H * 60 + M;
}

static void fillJamaah(PrayerDay& d){
  // Fajr +20, Dhuhr/Asr/Isha +15, Maghrib +10
  d.fajrJ    = (d.fajr    <0)?-1 : (d.fajr    + 20);
  d.dhuhrJ   = (d.dhuhr   <0)?-1 : (d.dhuhr   + 15);
  d.asrJ     = (d.asr     <0)?-1 : (d.asr     + 15);
  d.maghribJ = (d.maghrib <0)?-1 : (d.maghrib + 10);
  d.ishaJ    = (d.isha    <0)?-1 : (d.isha    + 15);
  // clamp wrap-around to next day (keep minutes possibly > 1440; we’ll mod when needed)
}

static String hijriPrettyFromJson(const JsonVariant& hijri){
  // hijri["day"], hijri["month"]["en"], hijri["year"]
  String d = hijri["day"] | "";
  String m = hijri["month"]["en"] | "";
  String y = hijri["year"] | "";
  // pad day
  if (d.length()==1) d = "0"+d;
  return d + " " + m + " " + y + " AH";
}

// Build /v1/timingsByCity/{date}?… per your working request
static String buildTimingsURL(int year, int month, int day) {
  String url = "https://api.aladhan.com/v1/timingsByCity/";
  url += ddmmyyyy(year, month, day);
  url += "?city=";  url += PRAYER_CITY;
  url += "&country="; url += PRAYER_COUNTRY;
  url += "&state="; url += PRAYER_STATE;
  url += "&method="; url += String(PRAYER_METHOD);
  url += "&shafaq="; url += PRAYER_SHAFAQ;
  url += "&tune=";   url += PRAYER_TUNE;
  url += "&timezonestring="; url += PRAYER_TZ;        // set to "UTC" to mirror your test
  url += "&calendarMethod="; url += PRAYER_CALMETHOD; // Hijri method
  return url;
}


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