#include "tuya_client.h"

#include "config.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace {

struct InternalZone {
  String id;
  TuyaZoneSnapshot out;
};

static TuyaSnapshot g_snapshot;
static InternalZone g_zones[TuyaSnapshot::MAX_ZONES];

static String prettyZoneLabel(const String& id) {
  String out = id;
  out.replace("_", " ");
  bool capNext = true;
  for (int i = 0; i < out.length(); ++i) {
    const char c = out[i];
    if (c == ' ') {
      capNext = true;
      continue;
    }
    if (capNext && c >= 'a' && c <= 'z') {
      out.setCharAt(i, char(c - 'a' + 'A'));
    }
    capNext = false;
  }
  return out;
}

static void resetSnapshot() {
  g_snapshot.zoneCount = 0;
  g_snapshot.cpuTemp = "--";
  g_snapshot.hasData = false;
  for (int i = 0; i < TuyaSnapshot::MAX_ZONES; ++i) {
    g_zones[i].id = "";
    g_zones[i].out.label = "";
    g_zones[i].out.temp = "--";
    g_zones[i].out.hum = "--";
    g_zones[i].out.bat = "--";
    g_snapshot.zones[i] = g_zones[i].out;
  }
}

static int findOrCreateZone(const String& zoneId) {
  for (int i = 0; i < g_snapshot.zoneCount; ++i) {
    if (g_zones[i].id == zoneId) return i;
  }
  if (g_snapshot.zoneCount >= TuyaSnapshot::MAX_ZONES) return -1;

  const int idx = g_snapshot.zoneCount++;
  g_zones[idx].id = zoneId;
  g_zones[idx].out.label = prettyZoneLabel(zoneId);
  g_zones[idx].out.temp = "--";
  g_zones[idx].out.hum = "--";
  g_zones[idx].out.bat = "--";
  g_snapshot.zones[idx] = g_zones[idx].out;
  return idx;
}

static bool fetchTuyaSnapshot() {
  if (WiFi.status() != WL_CONNECTED) {
    resetSnapshot();
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  const String url = String(PI_BASE_URL) + "/api/tuya";

  if (!http.begin(client, url)) {
    resetSnapshot();
    return false;
  }

  http.setTimeout(8000);
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    resetSnapshot();
    return false;
  }

  const String body = http.getString();
  http.end();

  DynamicJsonDocument doc(12288);
  if (deserializeJson(doc, body)) {
    resetSnapshot();
    return false;
  }

  resetSnapshot();

  JsonArray entities = doc["entities"].as<JsonArray>();
  if (entities.isNull()) return false;

  for (JsonVariant v : entities) {
    const char* eid = v["entity_id"] | "";
    const char* state = v["state"] | "";
    const char* unit = v["unit"] | "";
    const String entityId(eid);
    const String value(state);
    const String unitS(unit);

    if (entityId == "sensor.system_monitor_processor_temperature") {
      g_snapshot.cpuTemp = value + unitS;
      continue;
    }

    if (!entityId.startsWith("sensor.")) continue;

    String key = entityId.substring(7);
    String suffix;
    if (key.endsWith("_temperature")) suffix = "_temperature";
    else if (key.endsWith("_humidity")) suffix = "_humidity";
    else if (key.endsWith("_battery")) suffix = "_battery";
    else continue;

    String zoneId = key.substring(0, key.length() - suffix.length());
    if (zoneId.endsWith("_th06")) {
      zoneId.remove(zoneId.length() - 5);
    }

    const int idx = findOrCreateZone(zoneId);
    if (idx < 0) continue;

    const String formatted = value + unitS;
    if (suffix == "_temperature") g_zones[idx].out.temp = formatted;
    else if (suffix == "_humidity") g_zones[idx].out.hum = formatted;
    else if (suffix == "_battery") g_zones[idx].out.bat = formatted;

    g_snapshot.zones[idx] = g_zones[idx].out;
  }

  g_snapshot.hasData = (g_snapshot.zoneCount > 0) || (g_snapshot.cpuTemp != "--");
  return g_snapshot.hasData;
}

}  // namespace

bool refreshTuyaSnapshot() {
  return fetchTuyaSnapshot();
}

const TuyaSnapshot& getTuyaSnapshot() {
  return g_snapshot;
}
