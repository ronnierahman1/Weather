// WeatherGraph.h
#pragma once

#include <Arduino.h>
#include "globals.h"
#include "weather_data.h"

// Renders an alternative weather dashboard that focuses on a line
// graph of today's temperatures (using hourly data from WeatherState).
// This does not call ep.update() and assumes epaper.setRotation(0)
// has already been done by the caller.
void renderWeatherGraphDashboard(EPaper& ep, const WeatherState& state);
