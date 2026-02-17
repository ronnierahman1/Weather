// #pragma once
// #include "globals.h"
// #include "weather_data.h"

// // Draw the full dashboard and push a full update
// void renderAll(EPaper& epaper, const WeatherState& S, const String& lastUpdated);

// // Minute-by-minute clock box (partial update path)
// void drawClockBox(EPaper& epaper);

// // Hard clears to fight ghosting
// void fullClearOnce(EPaper& epaper);
// void deepClean(EPaper& epaper);

#pragma once
#include "globals.h"       // EPaper type + colors
#include "weather_data.h"
#include <Arduino.h>

void renderAll(EPaper& epaper, const WeatherState& S, const String& lastUpdated);
void renderAllDeepClean(EPaper& epaper, const WeatherState& S, const String& lastUpdated);
// void drawClockBox(EPaper& epaper, const WeatherState& S);

void fullClearOnce(EPaper& epaper);
void deepClean(EPaper& epaper);
// Full-screen salah timetable
void renderSalahTimetable(EPaper& epaper, const WeatherState& S, const String& lastUpdated);
tm nextPrayerTime(const WeatherState& S);
bool hasNextPrayerTimeElapsed(tm currentTime, tm nextTime);