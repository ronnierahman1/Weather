#include "pi_feed_client.h"
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

bool fetchStringListFromPi(
  const String url,
  const char* jsonArrayKey,
  String* outItems,
  int outCap,
  int& outCount,
  String* outUpdatedHHMM
){
  outCount = 0;
  if (outUpdatedHHMM) *outUpdatedHHMM = "";

  WiFiClient client;
  HTTPClient http;

  if (!http.begin(client, url)) return false;

  http.setTimeout(15000);
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) return false;

  if (outUpdatedHHMM && doc["updated_hhmm"].is<const char*>())
    *outUpdatedHHMM = String((const char*)doc["updated_hhmm"]);

  JsonArray arr = doc[jsonArrayKey].as<JsonArray>();
  if (arr.isNull()) return false;

  int n = 0;
  for (JsonVariant v : arr) {
    if (n >= outCap) break;
    if (v.is<const char*>()) outItems[n++] = String((const char*)v);
  }
  outCount = n;
  return outCount > 0;
}
