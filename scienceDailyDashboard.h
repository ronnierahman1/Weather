// scienceDailyDashboard.h
#pragma once

#include <Arduino.h>
#include "globals.h"     // EPaper
#include "newsFeeds.h"   // PageRenderMode
#include "rssFeedHandler.h"

const int numberOfScienceDailyNewsItems = 20;
// Backward-compat alias (your Weather.ino currently uses this name)
const int numberOfScienceNewsItems = numberOfScienceDailyNewsItems;

bool fetchScienceDailyNews(String outItems[numberOfScienceDailyNewsItems], int& outCount);

// New: fetch from Raspberry Pi (already parsed JSON)
bool fetchScienceDailyNewsFromPi(String outItems[numberOfScienceDailyNewsItems], int& outCount);

void renderScienceDailyNewsDashboard(
    EPaper& ep,
    const String items[numberOfScienceDailyNewsItems],
    int count,
    const String& lastUpdatedHHMM,
    PageRenderMode mode
);
