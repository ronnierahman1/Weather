#pragma once

#include <Arduino.h>

struct TuyaZoneSnapshot {
  String label;
  String temp;
  String hum;
  String bat;
};

struct TuyaSnapshot {
  static const int MAX_ZONES = 4;
  TuyaZoneSnapshot zones[MAX_ZONES];
  int zoneCount = 0;
  String cpuTemp = "--";
  bool hasData = false;
};

bool refreshTuyaSnapshot();
const TuyaSnapshot& getTuyaSnapshot();
