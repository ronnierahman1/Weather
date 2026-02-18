#include "driver.h"   // your original EPaper config
#include <TFT_eSPI.h> // Seeed_GFX entry (EPAPER_ENABLE path)
#include <WiFi.h>
#include <time.h>
#include <FS.h>
using fs::FS;
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
int sleep_seconds = 60000;
// Single EPaper instance visible to all .cpp through globals.h
EPaper epaper;

// Simple web server for timezone control
WebServer server(80);
// simple web auth
static const char* WEB_USER = "admin";
static const char* WEB_PASS = "psw1";
bool webAuthenticated = false;

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

void displayText(EPaper& epaper, const String &text, int x, int y, bool update = true)
{
  epaper.drawString(text, x, y);
  if (update) {
    epaper.update();
  }
}

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
    Serial.print("WiFi IP: ");
    Serial.println(ip);
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

typedef struct {
  String message;
  int x;
  int y;  
}fetchDataDisplay;

void fetchData(bool showProgress = true)
{
  // setInfoTextSettings(epaper);
  wifiConnect(showProgress);
  fetchDataDisplay fd[9];
  int x=20, y=70, lineSpacing=25;

  fd[0] =(fetchDataDisplay){"Fetching Data...", x, y};
  y += lineSpacing;
  // current + hourly
  // get the time it takes to fetch weather
  unsigned long start = millis();
  if(!fetchCurrentAndHourlyFromPi(state))
    fetchCurrentAndHourly(state); // current + hourly
  unsigned long duration = millis() - start;

  // displayTimeDate(epaper);
  fd[1] = (fetchDataDisplay){"Fetching Current + Hourly Weather... Done. Took " + String(duration) + "ms", x, y};


  // daily
  start = millis();
  if(!fetchDailyFromPi(state))
    fetchDaily(state); // 7-day daily
  duration = millis() - start;
  fd[2] = (fetchDataDisplay){"Fetching 7-Days' Weather... Done. Took " + String(duration) + "ms", x, y + lineSpacing};
  bindWeatherForDashboard(state);
  
  
  // prayers + hijri
  start = millis();
  if(!fetchPrayersAndHijriFromPi(state))
    fetchPrayersAndHijri(state); // prime at boot
  duration = millis() - start;
  fd[3] = (fetchDataDisplay){"Fetching Prayers + Hijri... Done. Took " + String(duration) + "ms", x, y + lineSpacing * 2};
  
  // fetchHackerNews
  start = millis();
  fetchHackerNewsFromPi(hackernewsTitles, hackernewsCount);
  duration = millis() - start;
  fd[4] = (fetchDataDisplay){"Fetching Hacker News... Done. Took " + String(duration) + "ms", x, y + lineSpacing * 3};

  start = millis();
  //make fetchXdaNews to report progress
  duration = millis() - start;
  fd[5] = (fetchDataDisplay){"Fetching XDA News... skipped. Work in progress. Took " + String(duration) + "ms", x, y + lineSpacing * 4};

  // fetchScienceDailyNews
  start = millis();
  fetchScienceDailyNewsFromPi(scienceDailyTitles, scienceDailyCount);
  duration = millis() - start;
  fd[6] = (fetchDataDisplay){"Fetching Science Daily News... Done. Took " + String(duration) + "ms", x, y + lineSpacing * 5};

  // fetchNewScientistNews
  start = millis();
  fetchNewScientistNewsFromPi(newScientistTitles, newScientistCount);
  duration = millis() - start;
  fd[7] = (fetchDataDisplay){"Fetching New Scientist News... Done. Took " + String(duration) + "ms", x, y + lineSpacing * 6};

  // fetchTheScientistNews
  start = millis();
  fetchTheScientistNewsFromPi(theScientistTitles, theScientistCount);
  duration = millis() - start;
  fd[8] = (fetchDataDisplay){"Fetching The Scientist News... Done. Took " + String(duration) + "ms", x, y + lineSpacing * 7};
  if(showProgress) {
  for(int i=0;i<9;i++){
    displayText(epaper, fd[i].message, fd[i].x, fd[i].y, false);
    }
  epaper.update();
  }
  updateLastUpdatedLabelFromNow();
}

bool fetchWeatherData()
{
  // Fetch weather data and update state
  // Weather every 15 minutes (full dashboard refresh)
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
    // require login
    bool authed = webAuthenticated;
    if (!authed && server.hasHeader("Cookie")) {
      String c = server.header("Cookie");
      if (c.indexOf("auth=1") >= 0) authed = true;
    }
    if (!authed) {
      String login = "<html><head><title>Login</title></head><body>";
      login += "<h2>Login</h2>";
      login += "<form method=\"POST\" action=\"/login\">";
      login += "Username: <input name=\"user\" type=\"text\"><br>";
      login += "Password: <input name=\"pass\" type=\"password\"><br>";
      login += "<input type=submit value=\"Log in\">";
      login += "</form></body></html>";
      Serial.println("[web] GET / -> login");
      server.send(200, "text/html", login);
      return;
    }

    struct tm ti;
    char buf[64];
    if (getLocalTime(&ti)) {
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ti);
    } else {
      strcpy(buf, "(time not available)");
    }
    int tz = getTimezoneHours();
    String html = "<html><head><title>XIAO Settings</title></head><body>";
    html += "<h2>XIAO Settings</h2>";
    html += String("<p>Local time: <b>") + String(buf) + "</b></p>";
    html += String("<p>Current Timezone (hours offset from UTC): <b>") + String(tz) + "</b></p>";
    html += "<form action=\"/set\" method=\"GET\">";
    html += "Set timezone (integer -12..+14): <input name=tz type=number step=1 value=\"" + String(tz) + "\"><br>";
    html += "Page pause (ms): <input name=\"sleep\" type=number step=100 value=\"" + String(sleep_seconds) + "\"><br>";
    html += "<input type=submit value=Set></form>";
    html += "<p><a href=\"/logout\">Logout</a></p>";
    html += "</body></html>";
    Serial.println("[web] GET /");
    server.send(200, "text/html", html);
  });

  server.on("/set", []() {
    if (!server.hasArg("tz")) {
      Serial.println("[web] /set missing tz");
      server.send(400, "text/plain", "Missing tz parameter");
      return;
    }
    String v = server.arg("tz");
    int tz = v.toInt();
    saveTimezoneHours(tz);
    if (server.hasArg("sleep")) {
      int s = server.arg("sleep").toInt();
      if (s < 1000) s = 1000;
      if (s > 600000) s = 600000;
      sleep_seconds = s;
      Serial.printf("[web] set sleep=%d\n", s);
    }
    // reconfigure NTP/localtime offset
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    Serial.printf("[web] set tz=%d\n", tz);
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  });

  server.on("/login", HTTP_POST, []() {
    String user = server.arg("user");
    String pass = server.arg("pass");
    if (user == String(WEB_USER) && pass == String(WEB_PASS)) {
      webAuthenticated = true;
      Serial.println("[web] login success");
      server.sendHeader("Set-Cookie", "auth=1; Path=/");
      server.sendHeader("Location", "/");
      server.send(303, "text/plain", "");
    } else {
      Serial.println("[web] login failed");
      String out = "<html><body><h3>Login failed</h3><p><a href=\"/\">Try again</a></p></body></html>";
      server.send(401, "text/html", out);
    }
  });

  server.on("/logout", []() {
    webAuthenticated = false;
    server.sendHeader("Set-Cookie", "auth=; Path=/; Max-Age=0");
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  });

  server.begin();
  Serial.println("[web] server.begin()");
  Serial.print("[web] WiFi.status(): "); Serial.println(WiFi.status());
  Serial.print("[web] Local IP: "); Serial.println(WiFi.localIP());
  updateLastUpdatedLabelFromNow();
  // Init per-minute tracking
  struct tm ti;
  if (getLocalTime(&ti))
  {
    lastDrawnMinute = ti.tm_min;
    lastDrawnYday = ti.tm_yday;
  }

  nextWeatherAt = millis() + WEATHER_INTERVAL_MS;
}

//------------------------------------------------------
// Arduino main loop
//------------------------------------------------------
void loop()
{
  // Non-blocking page scheduler:
  // - server.handleClient() runs constantly (instant web UI)
  // - only one heavy render happens per "sleep_seconds"
  // - second pages (Second_Page) skip deepClean()
  
  static bool inited = false;
  static uint32_t nextStepAtMs = 0;
  static uint8_t step = 0;

  server.handleClient();
  delay(1);

  const uint32_t nowMs = millis();

  if (!inited)
  {
    inited = true;
    nextStepAtMs = nowMs;
    step = 0;
  }

  if ((int32_t)(nowMs - nextStepAtMs) < 0)
    return;

  nextStepAtMs = nowMs + (uint32_t)sleep_seconds;

  PageRenderMode mode;

  switch (step)
  {
    case 0: 
      deepClean(epaper);
      displayMainWeather();      
      break;
    case 1:
      deepClean(epaper);
      displayWeatherGraphDashboard();
      break;

    case 2:
      wifi_turnoff(); // to save power during graph rendering
      mode = First_Page;
      deepClean(epaper);
      displayHackerNewsDashboard(mode);
      break;

    case 3:
      mode = Second_Page;
      displayHackerNewsDashboard(mode);
      break;

    case 4:
      deepClean(epaper);
      displaySalahDashboard();
      break;

    case 5:
      mode = First_Page;
      deepClean(epaper);
      displayScienceDailyNewsDashboard(mode);
      break;

    case 6:
      mode = Second_Page;
      displayScienceDailyNewsDashboard(mode);
      break;

    case 7:
      deepClean(epaper);
      displayWeatherDashboard();
      break;

    case 8:
      mode = First_Page;
      deepClean(epaper);
      displayTheScientistNewsDashboard(mode);
      break;

    case 9:
      mode = Second_Page;
      displayTheScientistNewsDashboard(mode);
      break;

    case 10:
      mode = First_Page;
      deepClean(epaper);
      displayNewScientistNewsDashboard(mode);
      break;

    case 11:
      mode = Second_Page;
      displayNewScientistNewsDashboard(mode);
      break;

    case 12:
      wifiConnect(false); // re-enable Wi-Fi for data fetching
      fetchData(false);
      updateLastUpdatedLabelFromNow();    
      step = 0; // reset to first page to show updated data immediately
      return; // skip incrementing step to show the new data immediately  
      break;
  }

  step = (uint8_t)((step + 1) % 14);
}
