#include "HtmlMarkupDecode.h"

// ---------- Internal data & helpers ----------

struct EntityMap {
  const char* name;   // entity name without '&' and ';'
  uint32_t    cp;     // Unicode codepoint
  const char* ascii;  // ASCII fallback; nullptr => emit UTF-8
};

// High-value subset; extend if you encounter more entities in feeds.
static const EntityMap kEntities[] = {
  {"amp",    0x0026, "&"},   {"lt",     0x003C, "<"},   {"gt",     0x003E, ">"},
  {"quot",   0x0022, "\""},  {"apos",   0x0027, "'"},  {"nbsp",   0x00A0, " "},

  {"ndash",  0x2013, "-"},   {"mdash",  0x2014, "--"}, {"hellip", 0x2026, "..."},
  {"lsquo",  0x2018, "'"},   {"rsquo",  0x2019, "'"},  {"ldquo",  0x201C, "\""},
  {"rdquo",  0x201D, "\""},  {"laquo",  0x00AB, "<<"}, {"raquo",  0x00BB, ">>"},

  {"copy",   0x00A9, "(c)"}, {"reg",    0x00AE, "(R)"},{"trade",  0x2122, "TM"},
  {"euro",   0x20AC, "EUR"}, {"pound",  0x00A3, "GBP"},{"yen",    0x00A5, "JPY"},
  {"bull",   0x2022, "*"},   {"middot", 0x00B7, "."},

  {"deg",    0x00B0, "deg"}, {"plusmn", 0x00B1, "+/-"},{"times",  0x00D7, "x"},
  {"divide", 0x00F7, "/"},   {"micro",  0x00B5, "u"},
  {"sup1",   0x00B9, "^1"},  {"sup2",   0x00B2, "^2"}, {"sup3",   0x00B3, "^3"},
  {"frac12", 0x00BD, "1/2"}, {"frac14", 0x00BC, "1/4"},{"frac34", 0x00BE, "3/4"},
};

// Encode a Unicode codepoint as UTF-8 (or ASCII fallback if requested).
static void appendCodepoint(String &out, uint32_t cp, bool asciiFallback, const char* ascii = nullptr) {
  if (asciiFallback && ascii) { out += ascii; return; }

  if (cp <= 0x7F) {
    out += char(cp);
  } else if (cp <= 0x7FF) {
    out += char(0xC0 | (cp >> 6));
    out += char(0x80 | (cp & 0x3F));
  } else if (cp <= 0xFFFF) {
    out += char(0xE0 | (cp >> 12));
    out += char(0x80 | ((cp >> 6) & 0x3F));
    out += char(0x80 | (cp & 0x3F));
  } else if (cp <= 0x10FFFF) {
    out += char(0xF0 | (cp >> 18));
    out += char(0x80 | ((cp >> 12) & 0x3F));
    out += char(0x80 | ((cp >> 6) & 0x3F));
    out += char(0x80 | (cp & 0x3F));
  }
}

static bool lookupEntity(const String &name, uint32_t &cp, const char* &ascii) {
  for (size_t i = 0; i < sizeof(kEntities)/sizeof(kEntities[0]); ++i) {
    if (name.equals(kEntities[i].name)) {
      cp    = kEntities[i].cp;
      ascii = kEntities[i].ascii;
      return true;
    }
  }
  return false;
}

static bool parseNumericEntity(const String &ent, uint32_t &cp) {
  if (ent.length() < 2 || ent[0] != '#') return false;

  if (ent.length() >= 3 && (ent[1] == 'x' || ent[1] == 'X')) {
    // Hex: #x....
    uint32_t v = 0;
    for (int i = 2; i < ent.length(); ++i) {
      char c = ent[i];
      uint8_t d = (c >= '0' && c <= '9') ? (c - '0') :
                  (c >= 'a' && c <= 'f') ? (c - 'a' + 10) :
                  (c >= 'A' && c <= 'F') ? (c - 'A' + 10) : 0xFF;
      if (d == 0xFF) return false;
      v = (v << 4) | d;
    }
    cp = v; return true;
  } else {
    // Decimal: #NNN
    uint32_t v = 0;
    for (int i = 1; i < ent.length(); ++i) {
      char c = ent[i];
      if (c < '0' || c > '9') return false;
      v = v * 10 + (c - '0');
    }
    cp = v; return true;
  }
}

// ---------- Public API ----------

String htmlDecode(const String &s, bool asciiFallback) {
  String out;
  out.reserve(s.length());  // conservative to avoid repeated realloc

  const int N = s.length();
  for (int i = 0; i < N; ) {
    char c = s[i];
    if (c != '&') { out += c; ++i; continue; }

    // Potential entity: find terminating ';' within a small window
    int semi = -1;
    const int maxLook = (i + 16 < N) ? (i + 16) : N;  // most entities < 16 chars
    for (int j = i + 1; j < maxLook; ++j) {
      if (s[j] == ';') { semi = j; break; }
      if (s[j] == ' ' || s[j] == '\n' || s[j] == '\r' || s[j] == '\t') break; // not an entity
    }

    if (semi < 0) { out += '&'; ++i; continue; }

    String ent = s.substring(i + 1, semi);  // "amp" or "#x2014" etc.
    uint32_t cp = 0;
    const char* ascii = nullptr;

    bool ok = parseNumericEntity(ent, cp) || lookupEntity(ent, cp, ascii);
    if (ok) {
      appendCodepoint(out, cp, asciiFallback, ascii);
      i = semi + 1;             // jump after ';'
    } else {
      // Unknown entity — keep '&' and move on
      out += '&';
      ++i;
    }
  }

  // Optional second pass to handle double-encoded numerics, e.g. &amp;#x2019;
  if (s.indexOf("&amp;#") >= 0) {
    return htmlDecode(out, asciiFallback);
  }
  return out;
}

