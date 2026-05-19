#include "settings.h"
#include "config.h"
#include <Preferences.h>

// Preferences namespace + key
static const char * PREF_NS = "weather";
static const char * KEY_TZ_HOURS = "tz_hours";

// Provide the global definition of GMT_OFFSET_SEC
long GMT_OFFSET_SEC = 3600L;

static Preferences prefs;

void initSettings()
{
  prefs.begin(PREF_NS, false);
  // read tz hours (int), default 1
  int tz = prefs.getInt(KEY_TZ_HOURS, 1);
  // clamp to reasonable range -12..+14
  if (tz < -12) tz = -12;
  if (tz > 14) tz = 14;
  GMT_OFFSET_SEC = (long)tz * 3600L;
}

int getTimezoneHours()
{
  return (int)(GMT_OFFSET_SEC / 3600L);
}

long getGmtOffsetSec()
{
  return GMT_OFFSET_SEC;
}

void saveTimezoneHours(int tzHours)
{
  if (tzHours < -12) tzHours = -12;
  if (tzHours > 14) tzHours = 14;
  prefs.putInt(KEY_TZ_HOURS, tzHours);
  GMT_OFFSET_SEC = (long)tzHours * 3600L;
}
