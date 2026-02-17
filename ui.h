#pragma once
#include "globals.h"       // EPaper type + colors
#include "weather_data.h"
#include <Arduino.h>
#include "newsFeeds.h"   // PageRenderMode

void renderAll(EPaper& epaper, const WeatherState& S, const String& lastUpdated);
void renderAllDeepClean(EPaper& epaper, const WeatherState& S, const String& lastUpdated);
void renderTopHalf(EPaper& epaper, const WeatherState& S, const String& lastUpdated, PageRenderMode mode);
void drawClockBox(EPaper& epaper, const WeatherState& S, bool clearFirst);

void fullClearOnce(EPaper& epaper);
void deepClean(EPaper& epaper);
String getNextSalahRemainingTimeText(const WeatherState& S, tm ti);
String CurrentLocalTime();