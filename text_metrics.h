#pragma once
#include <Arduino.h>

constexpr int GFX_BASE_CHAR_W = 6;  // Seeed/Adafruit GFX 1x font
constexpr int GFX_BASE_CHAR_H = 8;

inline int glyphWidth (int textSize)  { return GFX_BASE_CHAR_W * textSize; }
inline int glyphHeight(int textSize)  { return GFX_BASE_CHAR_H * textSize; }

inline int textWidth(const char* s, int textSize)  { return strlen(s) * glyphWidth(textSize); }
inline int textWidth(const String& s, int textSize){ return s.length() * glyphWidth(textSize); }
