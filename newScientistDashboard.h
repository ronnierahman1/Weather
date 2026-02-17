#pragma once
#include <Arduino.h>

class EPaper; // provided by your driver

// Tune like ScienceDaily
static constexpr int numberOfNewScientistNewsItems = 20;

// Fetch up to 70 items (title + description) from New Scientist RSS.
bool fetchNewScientistNews(String outItems[numberOfNewScientistNewsItems], int &outCount);

// bool fetchNewScientistNews_rssRead(String (*out), int& outCount);
bool fetchNewScientistNews_TinyXML(String (*out),int& outCount);

// New: fetch from Raspberry Pi (already parsed JSON)
bool fetchNewScientistNewsFromPi(String outItems[numberOfNewScientistNewsItems], int& outCount);

// Render titles (single-column, paginated like ScienceDaily). Numbers are prefixed (1., 2., …).
void renderNewScientistNewsDashboard(EPaper &ep,
                                     const String items[numberOfNewScientistNewsItems],
                                     int count,
                                     const String &lastUpdatedHHMM, PageRenderMode mode);
