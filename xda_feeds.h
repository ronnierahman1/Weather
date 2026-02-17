#pragma once
#include <Arduino.h>

// Keep fixed-size buffers in dashboards (e.g., 10 per section)
#ifndef XDA_MAX_TITLES
#define XDA_MAX_TITLES 10
#endif

// Add more as needed. Fixed enum keeps switch/arrays tiny and fast.
enum class XdaCat : uint8_t {
  PcHardware, Cpu, Storage, Monitors, InputDevices,
  Productivity, SoftwareOther, Windows, Linux, Devices,
  Gaming, SBC, Laptops, GamingHandhelds,
  PrebuiltPCs,  Networking, SmartHome, COUNT
};


struct XdaFeedSpec {
  const char* name;   // section header
  const char* url;    // RSS url
};

// Global registry (defined in .cpp)
extern const XdaFeedSpec kXdaFeeds[(int)XdaCat::COUNT];

// Fetch one category into a caller-owned buffer.
// Returns true on success, sets outCount (0..cap).
bool fetchXdaTitles(XdaCat cat, String* out, int cap, int& outCount);
bool fetchXdaTitles(XdaCat cat, String* outTitles,String* outDates, String* outAuthors, int cap, int& outCount);

// Convenience: fetch many categories in one call.
// 'cats' is an array of categories; 'outTitles' is an array of per-cat arrays.
// Example: outTitles[i] has capacity 'capEach' for cats[i] and outCounts[i] is filled.
bool fetchXdaTitlesMany(const XdaCat* cats, int nCats,
                        String (*outTitles)[XDA_MAX_TITLES], int capEach,
                        int* outCounts);
