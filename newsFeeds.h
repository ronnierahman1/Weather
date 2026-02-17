#pragma once
#include "globals.h"       // EPaper type + colors
#include <Arduino.h>

const int numberOfNewsItems = 20;
enum PageRenderMode { Full_Page=0, First_Page=1, Second_Page=2 };
void renderNewsDashboard(EPaper &ep,
                               const String titles[numberOfNewsItems], int count,
                               const String &lastUpdatedHHMM, PageRenderMode mode, int cols, String titlePrefix);