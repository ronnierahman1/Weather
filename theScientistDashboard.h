// theScientistDashboard.h
#pragma once

#include <Arduino.h>
#include "globals.h"     // EPaper
#include "newsFeeds.h"   // PageRenderMode

const int numberOfTheScientistNewsItems = 20;

bool fetchTheScientistNews(String outItems[numberOfTheScientistNewsItems], int& outCount);
bool fetchTheScientistNewsFromPi(String outItems[numberOfTheScientistNewsItems], int& outCount);

void renderTheScientistNewsDashboard(
    EPaper& ep,
    const String items[numberOfTheScientistNewsItems],
    int count,
    const String& lastUpdatedHHMM,
    PageRenderMode mode
);
