#pragma once
#include <Arduino.h>
#include "newsFeeds.h"

class EPaper;  // forward (from EPaper.h)



// Fetch the HN front page and fill up to 30 titles.
// Returns true on success. outCount will be 0..30.
bool fetchHackerNews(String outTitles[numberOfNewsItems], int &outCount);

bool fetchHackerNewsFromPi(String outTitles[numberOfNewsItems], int &outCount);

// Render the dashboard (3 columns x 10 rows) and push a full update.
void renderHackerNewsDashboard(EPaper &epaper,
                               const String titles[numberOfNewsItems], int count,
                               const String &lastUpdatedHHMM, PageRenderMode mode);
