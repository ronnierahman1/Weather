#include "driver.h"   // your original EPaper config
#include <TFT_eSPI.h> // Seeed_GFX entry (EPAPER_ENABLE path)
#include <WiFi.h>
#include <time.h>
#include <WebServer.h>

#include "settings.h"

#include "config.h"
#include "globals.h"
#include "text_metrics.h"
#include "weather_data.h"
#include "weather_api.h"
#include "weather_binding.h"
#include "ui.h"
#include "salah_ui.h"
#include "ui_weather.h"
#include "hackerNewsDashboard.h"
#include "xdaDevelopersNewsDashboard.h"
#include "scienceDailyDashboard.h"
#include "newScientistDashboard.h"
#include "TheScientistDashboard.h"
#include "newsFeeds.h"
#include "WeatherGraph.h"

extern "C"
{
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_bt.h"
}

// Global variables
tm nextSalahTime; // = nextPrayerTime(state);
int screen_full_refreshed = 0;
bool hasFetchedPrayerData = false;
bool hasFetchedWeatherData = false;
bool hasPrayerTimeElapsed = false;
int minute = 0;
int sleep_seconds = 58;
// Single EPaper instance visible to all .cpp through globals.h
EPaper epaper;

// Simple web server for timezone control
WebServer server(80);

// App state
WeatherState state;
String lastUpdatedLabel = "--:--";

// Scheduling
unsigned long nextWeatherAt = 0;
int lastDrawnMinute = -1;
int lastDrawnYday = -1;

static String hackernewsTitles[numberOfNewsItems];
static int hackernewsCount = 0;

static String xdaTitles[numberOfNewsItems];
static int xdaCount = 0;

static String scienceDailyTitles[numberOfScienceNewsItems];
static int scienceDailyCount = 0;
static String theScientistTitles[numberOfScienceNewsItems];
static int theScientistCount = 0;
static String newScientistTitles[numberOfScienceNewsItems];
static int newScientistCount = 0;
xdaNewsItem xdaNewsItems[numOfCategorys];

void setInfoTextSettings(EPaper& epaper)
{
  epaper.setTextSize(1);
  epaper.setTextColor(TFT_BLACK);
  epaper.setTextFont(2);
}

void displayText(EPaper& epaper, const String &text, int x, int y)
{
  epaper.drawString(text, x, y);
  epaper.update();
}

// void displayTimeDate(EPaper epaper)
// {
//   struct tm ti;
//   if (!getLocalTime(&ti))
//     return;
//   char timeStr[16];
//   strftime(timeStr, sizeof(timeStr), "%H:%M", &ti);
//   char dateStr[32];
//   strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y", &ti);
//   epaper.drawString(timeStr, 10, 0);
//   epaper.drawString(dateStr, 80, 0);
//   epaper.update();
// }

static String two(int v)
{
  char b[8];
  snprintf(b, sizeof(b), "%02d", v);
  return b;
}
static void updateLastUpdatedLabelFromNow()
{
  struct tm ti;
  if (!getLocalTime(&ti))
  {
    lastUpdatedLabel = "--:--";
    return;
  }
  lastUpdatedLabel = two(ti.tm_hour) + ":" + two(ti.tm_min);
}

// Helpful: readable formatted "last updated" for header
static String nowPretty()
{
  struct tm ti;
  if (!getLocalTime(&ti))
    return String("--:--");
  char buf[32];
  strftime(buf, sizeof(buf), "%H:%M", &ti);
  return String(buf);
}

static bool wifiConnect(bool showProgress = true)
{
  if(showProgress) displayText(epaper, "Connecting to Wi-Fi...", 20, 20);
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(1000);
  }
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000)
    delay(200);
  if (WiFi.status() == WL_CONNECTED)
  {
    String ip = WiFi.localIP().toString();
    if(showProgress) displayText(epaper, "Connected to Wi-Fi. IP: " + ip, 20, 40);
    delay(500);
    return true;
  }
  else{
    WiFi.begin(WIFI_SSID_5GHZ, WIFI_PASSWORD);
    delay(1000);
    t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000)
      delay(200);
    if (WiFi.status() == WL_CONNECTED)
    {
      String ip = WiFi.localIP().toString();
      if(showProgress) displayText(epaper, "Connected to Wi-Fi. IP: " + ip, 20, 40);
      delay(500);
      return true;
    }
  }

  if(showProgress) displayText(epaper, "Wi-Fi Connect Failed!", 20, 40);
  delay(1000);
  return false;
}

void wifi_turnoff()
{
  // 1) Close all clients first — do this in your code that owns them:
  // http.end(); client.stop(); etc.

  // 2) Disconnect and stop Wi-Fi stack
  WiFi.disconnect(true, true); // drop STA, erase creds, send deauth
  delay(50);
  esp_wifi_stop(); // stop driver task
  delay(20);
  WiFi.mode(WIFI_OFF); // put radio to OFF
  delay(20);

  // 3) (Optional) also stop BT if ever enabled
  // esp_bt_controller_disable();

  // 4) Yield to let lwIP timers run down
  // delay(50);
}

void initialize_epaper()
{
  epaper.begin();
  epaper.setRotation(4);
  epaper.fillScreen(TFT_WHITE);
  epaper.update();
}

void fetchData(bool showProgress = true)
{
  // setInfoTextSettings(epaper);
  wifiConnect(showProgress);
  int x=20, y=70, lineSpacing=25;
  if(showProgress) displayText(epaper, "Fetching Data...", x, y);

  y += lineSpacing;
  // current + hourly
  if(showProgress) displayText(epaper, "Fetching Current + Hourly Weather...", x, y);
  // get the time it takes to fetch weather
  unsigned long start = millis();
  if(!fetchCurrentAndHourlyFromPi(state))
    fetchCurrentAndHourly(state); // current + hourly
  unsigned long duration = millis() - start;
  // displayTimeDate(epaper);
  if(showProgress) displayText(epaper, "Fetching Current + Hourly Weather... Done. Took " + String(duration) + "ms", x, y);
  // displayTimeDate(epaper);
  // delay(100); // to avoid spamming


  // daily
  if(showProgress) displayText(epaper, "Fetching 7-Days' Weather...", x, y + lineSpacing);
  start = millis();
  if(!fetchDailyFromPi(state))
    fetchDaily(state); // 7-day daily
  duration = millis() - start;
  if(showProgress) displayText(epaper, "Fetching 7-Days' Weather... Done. Took " + String(duration) + "ms", x, y + lineSpacing);
  // delay(100); // to avoid spamming
  bindWeatherForDashboard(state);
  
  
  // prayers + hijri
  if(showProgress) displayText(epaper,"Fetching Prayers + Hijri...", x, y + lineSpacing * 2);
  start = millis();
  if(!fetchPrayersAndHijriFromPi(state))
    fetchPrayersAndHijri(state); // prime at boot
  duration = millis() - start;
  if(showProgress) displayText(epaper,"Fetching Prayers + Hijri... Done. Took " + String(duration) + "ms", x, y + lineSpacing * 2);
  // delay(100); // to avoid spamming
  
  // fetchHackerNews
  if(showProgress) displayText(epaper,"Fetching Hacker News...", x, y + lineSpacing * 3);
  start = millis();
  fetchHackerNewsFromPi(hackernewsTitles, hackernewsCount);
  duration = millis() - start;
  if(showProgress) displayText(epaper,"Fetching Hacker News... Done. Took " + String(duration) + "ms", x, y + lineSpacing * 3 );
  // delay(100); // to avoid spamming

  // fetchXdaNews(xdaTitles, xdaCount);
  if(showProgress) displayText(epaper,"Fetching XDA News...", x, y + lineSpacing * 4);
  start = millis();
  // //make fetchXdaNews to report progress
  // fetchXdaNews(xdaNewsItems, xdaCount,"Fetching XDA News...", x, y + lineSpacing * 4);
  duration = millis() - start;
  if(showProgress) displayText(epaper,"Fetching XDA News...                     skipped. Work in progress. Took " + String(duration) + "ms", x, y + lineSpacing * 4);
  // delay(100); // to avoid spamming

  // fetchScienceDailyNews
  if(showProgress) displayText(epaper,"Fetching Science Daily...", x, y + lineSpacing * 5);
  start = millis();
  fetchScienceDailyNewsFromPi(scienceDailyTitles, scienceDailyCount);
  duration = millis() - start;
  if(showProgress) displayText(epaper,"Fetching Science Daily... Done. Took " + String(duration) + "ms", x, y + lineSpacing * 5);
  // delay(100); // to avoid spamming

  // fetchNewScientistNews
  if(showProgress) displayText(epaper,"Fetching New Scientist...", 20, y + lineSpacing * 6);
  start = millis();
  fetchNewScientistNewsFromPi(newScientistTitles, newScientistCount);
  duration = millis() - start;
  if(showProgress) displayText(epaper,"Fetching New Scientist... Done. Took " + String(duration) + "ms", 20, y + lineSpacing * 6);
  // delay(100); // to avoid spamming

  // fetchTheScientistNews
  if(showProgress) displayText(epaper,"Fetching The Scientist...", x, y + lineSpacing * 7);
  start = millis();
  fetchTheScientistNewsFromPi(theScientistTitles, theScientistCount);
  duration = millis() - start;
  if(showProgress) displayText(epaper,"Fetching The Scientist... Done. Took " + String(duration) + "ms", x, y + lineSpacing * 7);
  
  updateLastUpdatedLabelFromNow();
  wifi_turnoff();
  // delay(100); // to avoid spamming
  if(showProgress) displayText(epaper,"Wifi Turned off...", x, y + lineSpacing * 8);
  delay(500);
}

bool fetchWeatherData()
{
  // Fetch weather data and update state
  // 2) Weather every 15 minutes (full dashboard refresh)
  bool ok1 = fetchCurrentAndHourly(state);
  bool ok2 = fetchDaily(state);
  bindWeatherForDashboard(state);
  return ok1 && ok2;
}

bool fetchedPrayerData()
{
  // Fetch prayer data and update state
  tm ti;
  if (getLocalTime(&ti))
  {
    if (ti.tm_hour >= 1 && state.lastPrayerFetchYday != ti.tm_yday)
    {
      if (fetchPrayersAndHijri(state))
      {
        state.lastPrayerFetchYday = ti.tm_yday;
        return true;
      }
      else
      {
        bool retryWorked = fetchPrayersAndHijri(state);
        state.lastPrayerFetchYday = ti.tm_yday;
        return true;
      }
    }
  }
  return false;
}

int getNextUpdateMinute(int min)
{
  return 0;
}

void displayMainWeather()
{
  renderAll(epaper, state, lastUpdatedLabel);
}

void displayWeatherGraphDashboard()
{
  renderWeatherGraphDashboard(epaper, state);
  epaper.update();
}

void displaySalahDashboard()
{
  renderSalahTimetable(epaper, state, nowPretty());
  epaper.update();
}

void displayWeatherDashboard()
{
  renderWeatherDashboard(epaper, state);
  epaper.update();
}

void displayHackerNewsDashboard(PageRenderMode mode)
{
  if(mode != Full_Page)
  {
    renderTopHalf(epaper, state, lastUpdatedLabel, mode);
  }
  renderHackerNewsDashboard(epaper, hackernewsTitles, hackernewsCount, lastUpdatedLabel, mode);
}

void displayXdaDevelopersNewsDashboard()
{
  renderXdaDevelopersNewsDashboard(epaper, xdaNewsItems, xdaCount, lastUpdatedLabel);
}

void displayScienceDailyNewsDashboard(PageRenderMode mode)
{
  if(mode != Full_Page)
  {
    renderTopHalf(epaper, state, lastUpdatedLabel, mode);
  }
  renderScienceDailyNewsDashboard(epaper, scienceDailyTitles, scienceDailyCount, lastUpdatedLabel, mode);
}

void displayTheScientistNewsDashboard(PageRenderMode mode)
{
  if(mode != Full_Page)
  {
    renderTopHalf(epaper, state, lastUpdatedLabel, mode);
  }
  renderTheScientistNewsDashboard(epaper, theScientistTitles, theScientistCount, lastUpdatedLabel, mode);
}

void displayNewScientistNewsDashboard(PageRenderMode mode)
{
  if(mode != Full_Page)
  {
    renderTopHalf(epaper, state, lastUpdatedLabel, mode);
  }
  renderNewScientistNewsDashboard(epaper, newScientistTitles, newScientistCount, lastUpdatedLabel, mode);
}

// ------------------------------------------------------

//------------------------------------------------------
// Arduino setup + loop
//------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println("Weather Dashboard Starting...");
  // load persisted settings (timezone)
  initSettings();
  initialize_epaper();
  setInfoTextSettings(epaper);
  if (!wifiConnect())
  {
    // fallback: draw last known / placeholder
    state.location = "Offline";
    state.temp = 0;
    state.feelsLike = 0;
    state.humidity = 0;
    state.condition = "—";
    renderAll(epaper, state, lastUpdatedLabel);
    epaper.update();
    return;
  }

  // unsigned long t0 = millis();
  // while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(200);

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  deepClean(epaper);
  // First fetch
  fetchData();
  // ensure Wi-Fi is active for the web server
  wifiConnect();

  // start web server to allow timezone viewing/setting
  server.on("/", []() {
    struct tm ti;
    char buf[64];
    if (getLocalTime(&ti)) {
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ti);
    } else {
      strcpy(buf, "(time not available)");
    }
    int tz = getTimezoneHours();
    String html = "<html><head><title>XIAO Time</title></head><body>";
    html += "<h2>Device Time</h2>";
    html += String("<p>Local time: <b>") + String(buf) + "</b></p>";
    html += String("<p>Timezone (hours offset from UTC): <b>") + String(tz) + "</b></p>";
    html += "<form action=\"/set\" method=\"GET\">";
    html += "Set timezone (integer -12..+14): <input name=tz type=number step=1 value=\"" + String(tz) + "\">";
    html += "<input type=submit value=Set></form>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/set", []() {
    if (!server.hasArg("tz")) {
      server.send(400, "text/plain", "Missing tz parameter");
      return;
    }
    String v = server.arg("tz");
    int tz = v.toInt();
    saveTimezoneHours(tz);
    // reconfigure NTP/localtime offset
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  });

  server.begin();
  updateLastUpdatedLabelFromNow();
  // Init per-minute tracking
  struct tm ti;
  if (getLocalTime(&ti))
  {
    lastDrawnMinute = ti.tm_min;
    lastDrawnYday = ti.tm_yday;
  }

  // Initial full render
  deepClean(epaper);
  displayMainWeather();
  // displayWeatherGraphDashboard();
  sleep(sleep_seconds);    

  nextWeatherAt = millis() + WEATHER_INTERVAL_MS;
}

//------------------------------------------------------
// Arduino main loop
//------------------------------------------------------
void loop()
{
  PageRenderMode mode;// = First_Page;
    deepClean(epaper);
    displayWeatherGraphDashboard();
    server.handleClient();
    sleep(sleep_seconds);

    mode = First_Page;
    deepClean(epaper);
    displayHackerNewsDashboard(mode);
    server.handleClient();
    sleep(sleep_seconds);

    mode = Second_Page;
    // deepClean(epaper);
    displayHackerNewsDashboard(mode);
    server.handleClient();
    sleep(sleep_seconds);

    // mode = Full_Page;
    // deepClean(epaper);
    // displayXdaDevelopersNewsDashboard();
    // sleep(sleep_seconds);

    deepClean(epaper);
    displaySalahDashboard();
    server.handleClient();
    sleep(sleep_seconds);    

    mode = First_Page;
    deepClean(epaper);
    displayScienceDailyNewsDashboard(mode);
    server.handleClient();
    sleep(sleep_seconds);

    mode = Second_Page;
    // deepClean(epaper);
    displayScienceDailyNewsDashboard(mode);
    server.handleClient();
    sleep(sleep_seconds);
    
    deepClean(epaper);
    displayWeatherDashboard();
    server.handleClient();
    sleep(sleep_seconds);
    
    // deepClean(epaper);
    //displayTheScientistNewsDashboard(mode);
    // sleep(sleep_seconds);
    
    mode = First_Page;
    deepClean(epaper);
    displayTheScientistNewsDashboard(mode);
    server.handleClient();
    sleep(sleep_seconds);
    
    mode = Second_Page;
    // deepClean(epaper);
    displayTheScientistNewsDashboard(mode);
    server.handleClient();
    sleep(sleep_seconds);
    
     mode = Full_Page;
    // displayNewScientistNewsDashboard();
    deepClean(epaper);
    displayMainWeather();
    server.handleClient();
    sleep(sleep_seconds);

    mode = First_Page;
    deepClean(epaper);
    displayNewScientistNewsDashboard(mode);
    sleep(sleep_seconds);
    
    mode = Second_Page;
    // deepClean(epaper);
    displayNewScientistNewsDashboard(mode);
    sleep(sleep_seconds);
    
  fetchData(false);
  updateLastUpdatedLabelFromNow();
  deepClean(epaper);
}
