#pragma once
#include <Adafruit_GFX.h>

// ---- Metrics we return ----
struct TextMetrics {
  int16_t w;       // visible width
  int16_t h;       // visible height
  int16_t ascent;  // px above baseline
  int16_t descent; // px below baseline (>=0)
  int16_t advance; // cursor advance (sum xAdvance)
};


// inline TextMetrics measureWithFont(const char* s, const GFXfont* f) {
//   TextMetrics m{0,0,0,0,0}; if (!s || !*s || !f) return m;
//   int32_t cx=0; int16_t minX=32767, minY=32767, maxX=-32768, maxY=-32768;
//   for (const char* p=s; *p; ++p) {
//     uint8_t c=(uint8_t)*p;
//     if (c < f->first || c > f->last) { cx += f->yAdvance/4; continue; }
//     const GFXglyph& g = f->glyph[c - f->first];
//     const int16_t gx = (int16_t)(cx + g.xOffset);
//     const int16_t gy = g.yOffset;
//     if (g.width && g.height) {
//       if (gx < minX) minX = gx; if (gy < minY) minY = gy;
//       if (gx + g.width  > maxX) maxX = gx + g.width;
//       if (gy + g.height > maxY) maxY = gy + g.height;
//     }
//     cx += g.xAdvance;
//   }
//   if (minX == 32767) return m;
//   const int16_t visW = (maxX > 0 ? maxX : 0) - (minX < 0 ? minX : 0);
//   const int16_t visH = (maxY - minY);
//   const int16_t asc  = (minY < 0) ? -minY : 0;
//   const int16_t des  = (maxY > 0) ?  maxY : 0;
//   m = {visW, visH, asc, des, (int16_t)cx}; return m;
// }

// Compute single-line bounds for ASCII text using a FreeFont
inline TextMetrics measureWithFont(const char* s, const GFXfont* f) {
  TextMetrics m{0,0,0,0,0};
  if (!s || !*s || !f) return m;

  int32_t cx = 0;                       // baseline cursor x
  int16_t minX =  32767, minY =  32767;
  int16_t maxX = -32768, maxY = -32768;

  for (const char* p = s; *p; ++p) {
    uint8_t c = (uint8_t)*p;
    if (c < f->first || c > f->last) {  // unknown char: advance a small space
      cx += f->yAdvance / 4;
      continue;
    }
    const GFXglyph& g = f->glyph[c - f->first];

    const int16_t gx = (int16_t)(cx + g.xOffset);  // glyph box top-left (baseline coords)
    const int16_t gy = g.yOffset;

    if (g.width && g.height) {
      if (gx < minX) minX = gx;
      if (gy < minY) minY = gy;
      if (gx + g.width  > maxX) maxX = gx + g.width;
      if (gy + g.height > maxY) maxY = gy + g.height;
    }
    cx += g.xAdvance;
  }

  if (minX == 32767) return m;          // nothing measurable

  // Visible bounds include left/right bearings properly
  const int16_t visW = (maxX > 0 ? maxX : 0) - (minX < 0 ? minX : 0);
  const int16_t visH = (maxY - minY);
  const int16_t asc  = (minY < 0) ? -minY : 0;
  const int16_t des  = (maxY > 0) ?  maxY : 0;

  m.w = visW; m.h = visH; m.ascent = asc; m.descent = des; m.advance = (int16_t)cx;
  return m;
}
