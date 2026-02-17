#pragma once
#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "ui.h"
#include "config.h"
#include "text_metrics.h"
#include "icons.h"
#include "hackerNewsDashboard.h"
#include "gfx_metrics.h"
#include "fonts_data.h"

// Basic glyph metrics for the default GFX bitmap font (tuned for your HN page)
static inline int charW(int sz) { return 7 * sz; }
static inline int charW2(int sz) { return 5 * sz; }
static inline int charH(int sz) { return 10 * sz; }
static inline int charH2(int sz) { return 8 * sz; }

String extractTag(const String &xml, int openPos, const String &tag);
int parseRssTitles(const String &rss, String (*out), int count, String trailingSiteName);
int printBoldTextLine(EPaper &ep, const String &text, int x, int y,
              int colW, int lineGap);
int printNormalTextLine(EPaper &ep, const String &text, int x, int y,
                    int colW, int lineGap);
int printObliqueTextLine(EPaper &ep,  const String &text, int x, int y,
              int colW, int lineGap);
void printHeaderText(EPaper &ep, const String header, int headH);
void printDottedLine(EPaper &ep, int y, int x0, int x1);
int printTextLine(EPaper &ep, const String &s, int x, int y,
             int colW, int textSize, int lineGap, int cw);
bool httpGET_Insecure(const String &url, String &payload);

int parseRssItems(const String &xml,
                  String(*out),
                  int capacity,
                  const String &trailingSiteName);
// --------- Generic RSS parser: <item><title>..</title><description>..</description> ----------
int parseRssItems(const String &xml,
                  String (*out)[2], int capacity,
                  const String &trailingSiteName);                  
int parseRssByTag(const String &rss, String (*out), int capacity, String tag, String trailingSiteName);                  

int parseRssTitlesAndDesc(const String &rss, String (*out)[2], int capacity, String trailingSiteName);

/// Parse up to `capacity` Atom entries. Fills out[i][0]=title, out[i][1]=summary/content.
int parseAtomEntries(const String &xml,
                        String (*out),
                        int capacity,
                        const String &trailingSiteName);