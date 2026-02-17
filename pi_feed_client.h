#pragma once
#include <Arduino.h>

bool fetchStringListFromPi(
  const String url,
  const char* jsonArrayKey,
  String* outItems,
  int outCap,
  int& outCount,
  String* outUpdatedHHMM // optional, can be nullptr
);
