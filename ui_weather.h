#pragma once
#include "globals.h"
// #include "weather_data.h"

void renderWeatherDashboard(EPaper& epaper, const WeatherState& S);
void renderWeatherDataTable(EPaper& epaper, const WeatherState& S, int x, int y, int w);