#pragma once
#include "rssFeedHandler.h"
#include "HtmlMarkupDecode.h"
#include "Fonts/FreeSansOblique8pt7b.h"

// Extract text inside a tag, tolerant of <![CDATA[...]]>
String extractTag(const String &xml, int openPos, const String &tag)
{
    // openPos points to '<' of <tag>
    int start = xml.indexOf('>', openPos);
    if (start < 0)
        return String();
    int close = xml.indexOf("</" + tag + ">", start + 1);
    if (close < 0)
        return String();
    String raw = xml.substring(start + 1, close);
    // Strip CDATA if present
    if (raw.startsWith(F("<![CDATA[")))
    {
        int endC = raw.indexOf(F("]]>"));
        if (endC > 9)
            raw = raw.substring(9, endC);
    }
    raw.trim();
    return raw;
}

int parseRssTitles(const String &rss, String (*out), int capacity, String trailingSiteName = "")
{
    int count = 0;
    int pos = 0;
    while (count < capacity)
    {
        int itemOpen = rss.indexOf(F("<item>"), pos);
        if (itemOpen < 0)
            break;
        int titleOpen = rss.indexOf(F("<title>"), itemOpen);
        if (titleOpen < 0)
        {
            pos = itemOpen + 6;
            continue;
        }
        String title = extractTag(rss, titleOpen, F("title"));

        title = htmlDecode(title);
        title.replace(F("\n"), F(" "));
        title.trim();
        if (title.length())
        {
            // Optional: remove trailing site suffixes if present
            // e.g., "Something - XDA Developers"
            if (trailingSiteName.length() > 0)
            {
                const String suffix = trailingSiteName;
                if (title.endsWith(suffix))
                    title.remove(title.length() - suffix.length());
            }
            out[count] = title;
            count++;
        }
        pos = titleOpen + 7;
    }
    return count;
}

int parseRssByTag(const String &rss, String (*out), int capacity, String tag, String trailingSiteName = "")
{
    int count = 0;
    int pos = 0;
    while (count < capacity)
    {
        int itemOpen = rss.indexOf(F("<item>"), pos);
        if (itemOpen < 0)
            break;
        int titleOpen = rss.indexOf("<"+tag+">", itemOpen);
        if (titleOpen < 0)
        {
            pos = itemOpen + 6;
            continue;
        }
        String title = extractTag(rss, titleOpen, tag);

        title = htmlDecode(title);
        title.replace(F("\n"), F(" "));
        title.trim();
        if (title.length())
        {
            // Optional: remove trailing site suffixes if present
            // e.g., "Something - XDA Developers"
            if (trailingSiteName.length() > 0)
            {
                const String suffix = trailingSiteName;
                if (title.endsWith(suffix))
                    title.remove(title.length() - suffix.length());
            }
            out[count] = title;
            count++;
        }
        pos = titleOpen + 7;
    }
    return count;
}

int parseRssTitlesAndDesc(const String &rss, String (*out)[2], int capacity, String trailingSiteName = "")
{
    int count = 0;
    int pos = 0;
    while (count < capacity)
    {
        int itemOpen = rss.indexOf(F("<item>"), pos);
        if (itemOpen < 0)
            break;
        int titleOpen = rss.indexOf(F("<title>"), itemOpen);
        if (titleOpen < 0)
        {
            pos = itemOpen + 6;
            continue;
        }
        String title = extractTag(rss, titleOpen, F("title"));
        int descOpen = rss.indexOf(F("<description>"), itemOpen);
        if (descOpen < 0)
        {
            pos = itemOpen + 6;
            continue;
        }
        String description = extractTag(rss, descOpen, F("description"));

        title = htmlDecode(title);
        title.replace(F("\n"), F(" "));
        title.trim();
        description = htmlDecode(description);
        description.replace(F("\n"), F(" "));
        description.trim();
        if (title.length())
        {
            // Optional: remove trailing site suffixes if present
            // e.g., "Something - XDA Developers"
            if (trailingSiteName.length() > 0)
            {
                const String suffix = trailingSiteName;
                if (title.endsWith(suffix))
                    title.remove(title.length() - suffix.length());
                if (description.endsWith(suffix))
                    description.remove(description.length() - suffix.length());
            }
            out[count][0] = title;
            out[count][1] = description;
            count++;
        }
        pos = titleOpen + 7;
    }
    return count;
}

void addPunctuation(String &s)
{
    if (s.length() > 0 && s.charAt(s.length() - 1) != '.' && s.charAt(s.length() - 1) != '?')
    {
        s += '.';
    }
}

// Word-wrap drawing into a fixed width column FreeSansOblique8pt7b
int printBoldTextLine(EPaper &ep,  const String &text, int x, int y,
              int colW, int lineGap = 2)
{

    String s = text;
    addPunctuation(s);
    ep.setFreeFont(&FreeSansBold8pt7b);
    int lineY = printTextLine(ep, s, x, y, colW, 1, lineGap, charW(1) + 1);
    return lineY;
}

// Word-wrap drawing into a fixed width column 
int printObliqueTextLine(EPaper &ep,  const String &text, int x, int y,
              int colW, int lineGap = 2)
{

    String s = text;
    addPunctuation(s);
    ep.setTextSize(1);
    // ep.setTextFont(1);
    ep.setFreeFont(&FreeSansOblique8pt7b);
    int lineY = printTextLine(ep, s, x, y, colW, 1, lineGap, charW(1) + 1);
    return lineY;
}

// Word-wrap drawing into a fixed width column
int printNormalTextLine(EPaper &ep, const String &text, int x, int y, int colW, int lineGap = 2)
{
    //check if the last char in a string is not '.' or '?' add a full stop
    String s = text;
    addPunctuation(s);
    ep.setFreeFont(&FreeSans8pt7b);
    int lineY = printTextLine(ep, s, x, y, colW, 1, lineGap, charW(1) + 1);
    return lineY;
}

void printHeaderText(EPaper &ep, const String header, int headH)
{
    ep.setFreeFont(&FreeSansBold9pt7b);
    ep.drawString(header, 10, 6);
    ep.drawFastHLine(0, headH, 800, TFT_BLACK);
}

void printDottedLine(EPaper &ep, int y, int x0, int x1)
{
    for (int x = x0; x < x1; x += 8)
    {
        ep.drawPixel(x, y, TFT_BLACK);
    }
}

int printTextLine(EPaper &ep, const String &s, int x, int y,
             int colW, int textSize, int lineGap, int cw)
{
    int ch = charH(1);
    int maxChars = max(1, (colW - 2) / cw);
    int lineY = y;

    int start = 0;
    while (start < s.length())
    {
        int end = start, lastSpace = -1, taken = 0;
        while (end < s.length() && taken < maxChars)
        {
            if (s[end] == ' ')
                lastSpace = end;
            end++;
            taken++;
        }
        int cut = (end == s.length() || s[end] == ' ')
                      ? end
                      : (lastSpace > start ? lastSpace : end);
        ep.drawString(s.substring(start, cut), x, lineY);
        lineY += ch + lineGap + 5;
        start = (cut == end) ? end : cut + 1;
    }
    return lineY;
}


// Robust GET for RSS/Atom over TLS (handles chunked). No gzip.
bool httpGET_Insecure(const String& url, String& payload)
{
  payload = "";

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, url)) {
    return false;
  }

  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setTimeout(20000);
  http.setReuse(false);
  http.useHTTP10(true);  // be conservative for some CDNs

  // Ask for plain XML, no compression
  http.addHeader(F("Accept"),
                 F("application/atom+xml, application/rss+xml, application/xml;q=0.9, */*;q=0.8"));
  http.addHeader(F("Accept-Encoding"), F("identity"));
  http.addHeader(F("Connection"), F("close"));
  http.addHeader(F("User-Agent"), F("ESP32C3-WeatherDash/1.0"));

  int code = http.GET();
  if (code != HTTP_CODE_OK) { http.end(); return false; }

  // Stream body (works for chunked responses)
  WiFiClient* s = http.getStreamPtr();
  payload.reserve(32768);
  uint8_t buf[513];

  unsigned long lastData = millis();
  while (http.connected() || s->available()) {
    int n = s->read(buf, sizeof(buf) - 1);
    if (n > 0) {
      buf[n] = 0;                 // XML is text; safe to NUL-terminate
      payload += (char*)buf;      // append this chunk
      lastData = millis();
    } else {
      // protection against some servers keeping the connection open
      if (millis() - lastData > 3000) break;
      delay(1);
    }
  }
  http.end();

  return payload.length() > 0;
}


// rssFeedHandler.cpp  (place these after extractTag/htmlDecode)

static inline void stripSuffix(String &s, const String &suffix)
{
    if (suffix.length() && s.endsWith(suffix))
        s.remove(s.length() - suffix.length());
}

// --------- Title-only RSS parser: <item><title>..</title> ----------
int parseRssItems(const String &xml,
                  String (*out), int capacity,
                  const String &trailingSiteName)
{
    int count = 0;
    int pos = 0;

    while (count < capacity)
    {
        int itemOpen = xml.indexOf(F("<item"), pos);
        if (itemOpen < 0)
            break;
        int itemStart = xml.indexOf('>', itemOpen);
        if (itemStart < 0)
            break;
        int itemClose = xml.indexOf(F("</item>"), itemStart);
        if (itemClose < 0)
            break;

        // title
        int tOpen = xml.indexOf(F("<title"), itemStart);
        String title;
        if (tOpen > 0 && tOpen < itemClose)
            title = extractTag(xml, tOpen, F("title"));

        title = htmlDecode(title);
        title.replace("\n", " ");
        title.trim();

        stripSuffix(title, trailingSiteName);

        if (title.length())
        {
            out[count] = title;
            ++count;
        }
        pos = itemClose + 7; // after </item>
    }
    return count;
}

// --------- Generic RSS parser: <item><title>..</title><description>..</description> ----------
int parseRssItems(const String &xml,
                  String (*out)[2], int capacity,
                  const String &trailingSiteName)
{
    int count = 0;
    int pos = 0;

    while (count < capacity)
    {
        int itemOpen = xml.indexOf(F("<item"), pos);
        if (itemOpen < 0)
            break;
        int itemStart = xml.indexOf('>', itemOpen);
        if (itemStart < 0)
            break;
        int itemClose = xml.indexOf(F("</item>"), itemStart);
        if (itemClose < 0)
            break;

        // title
        int tOpen = xml.indexOf(F("<title"), itemStart);
        String title;
        if (tOpen > 0 && tOpen < itemClose)
            title = extractTag(xml, tOpen, F("title"));

        // prefer <description>, fallback to <content:encoded>
        String desc;
        int dOpen = xml.indexOf(F("<description"), itemStart);
        if (dOpen > 0 && dOpen < itemClose)
        {
            desc = extractTag(xml, dOpen, F("description"));
        }
        else
        {
            int cOpen = xml.indexOf(F("<content:encoded"), itemStart);
            if (cOpen > 0 && cOpen < itemClose)
                desc = extractTag(xml, cOpen, F("content:encoded"));
        }

        title = htmlDecode(title);
        title.replace("\n", " ");
        title.trim();
        desc = htmlDecode(desc);
        desc.replace("\n", " ");
        desc.trim();

        stripSuffix(title, trailingSiteName);
        stripSuffix(desc, trailingSiteName);

        if (title.length())
        {
            out[count][0] = title;
            out[count][1] = desc;
            ++count;
        }
        pos = itemClose + 7; // after </item>
    }
    return count;
}


// --------- Generic ATOM parser: <entry><title>..</title><summary>|<content>.. ----------
int parseAtomEntries(const String &xml,
                     String (*out), int capacity,
                     const String &trailingSiteName)
{
    int count = 0;
    int pos = 0;

    while (count < capacity)
    {
        int eOpen = xml.indexOf(F("<entry"), pos);
        if (eOpen < 0)
            break;
        int eStart = xml.indexOf('>', eOpen);
        if (eStart < 0)
            break;
        int eClose = xml.indexOf(F("</entry>"), eStart + 1);
        if (eClose < 0)
            break;

        // title
        String title;
        int tOpen = xml.indexOf(F("<title"), eStart);
        if (tOpen > 0 && tOpen < eClose)
            title = extractTag(xml, tOpen, F("title"));

        // // summary or content
        // String summary;
        // int sOpen = xml.indexOf(F("<summary"), eStart);
        // if (sOpen > 0 && sOpen < eClose)
        // {
        //     summary = extractTag(xml, sOpen, F("summary"));
        // }
        // else
        // {
        //     int cOpen = xml.indexOf(F("<content"), eStart);
        //     if (cOpen > 0 && cOpen < eClose)
        //         summary = extractTag(xml, cOpen, F("content"));
        // }

        title = htmlDecode(title);
        title.replace("\n", " ");
        title.trim();
        // summary = htmlDecode(summary);
        // summary.replace("\n", " ");
        // summary.trim();

        stripSuffix(title, trailingSiteName);
        // stripSuffix(summary, trailingSiteName);

        if (title.length())
        {
            out[count] = title;
            // out[count][1] = summary;
            ++count;
        }
        pos = eClose + 8;
    }
    return count;
}
