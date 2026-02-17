#pragma once
#include "weather_data.h"

// Turn raw Open-Meteo fields in WeatherState into the exact fields
// that ui_weather.cpp already expects (temp/feelsLike/humidity/condition,
// today{phrase,low,high,uv}, tomorrow{...}, location/updated).
void bindWeatherForDashboard(WeatherState& S);
String getUvString(float uv);