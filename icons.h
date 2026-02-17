#pragma once
#include "globals.h"   // EPaper + colors available here

// Weather icon types
enum IconType { SUN, PARTLY, CLOUD, RAIN, STORM, SNOW, FOG, WINDY };
enum FontSize { SIZE_9PT, SIZE_12PT, SIZE_24PT, SIZE_36PT, SIZE_48PT, SIZE_9PT_BOLD, SIZE_12PT_BOLD, SIZE_24PT_BOLD, SIZE_36PT_BOLD, SIZE_48PT_BOLD };

// Map WMO → IconType
IconType iconForWMO(int code, bool isDay);

// Draw weather icons
void drawWeatherIcon(IconType t, int cx, int cy, int s);
void drawWeatherIconTL(IconType t, int left, int top, int s);

// Sunrise/Sunset (y is horizon baseline)
void drawSunriseIcon(int x, int y, int s, int m = 1);
void drawSunsetIcon (int x, int y, int s, int m = 1);

// “NN°C”
void drawTempDegC(int x, int y, float t, int textSize, int textPoint);
int  tempDegCWidth(float t, int textPoint);

void drawUvIcon(int x, int y, int s);

// TL anchor box size
extern const int ICON_BOX_BASE;
