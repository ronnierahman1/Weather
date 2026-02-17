// newScientistDashboard.cpp
// New Scientist (RSS) dashboard — mirrors scienceDailyDashboard style, reusing rssFeedHandler.
#pragma once
#include <Arduino.h>
#include "rssFeedHandler.h" // htmlDecode, extractTag, drawtitle, printHeaderText, printNormalTextLine, httpGET_Insecure
#include "newScientistDashboard.h"
#include <WiFiClientSecure.h>
#include <WiFiClient.h> // for WiFiClient* from http.getStreamPtr()
#include <HTTPClient.h>
#include <TinyXML.h>
#include "HtmlMarkupDecode.h"
#include "newsFeeds.h"
#include "pi_feed_client.h"
#include "config.h"


static const String PI_NEWSCI_URL = String(PI_BASE_URL) + "/api/newscientist";

bool fetchNewScientistNewsFromPi(String outItems[numberOfNewScientistNewsItems], int& outCount)
{
  return fetchStringListFromPi(
    PI_NEWSCI_URL,
    "items",
    outItems,
    numberOfNewScientistNewsItems,
    outCount,
    nullptr
  );
}


static bool fetchNS_XML(String &body)
{
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    if (!http.begin(client, "https://www.newscientist.com/feed/home/"))
        return false;

    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setTimeout(20000); // be generous
    http.setReuse(false);
    http.addHeader("Accept", "application/rss+xml, application/xml;q=0.9,*/*;q=0.8");
    http.addHeader("Accept-Encoding", "identity"); // avoid gzip
    http.addHeader("Connection", "close");
    http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0) AppleWebKit/537.36 Chrome/124 Safari/537.36");

    int code = http.GET();
    if (code <= 0)
    {
        http.end();
        return false;
    }

    // Stream the body (works for chunked responses too)
    WiFiClient *s = http.getStreamPtr();
    body.reserve(32768); // pre-allocate some space
    uint8_t buf[513];
    while (http.connected())
    {
        int n = s->read(buf, 512);
        if (n <= 0)
        {
            delay(1);
            if (!s->connected())
                break;
            continue;
        }
        buf[n] = 0;
        body += (char *)buf; // append this chunk
    }
    http.end();

    if (body.length() == 0)
        return body.length() > 0;
}

// ---- TinyXML context + callback ----
struct RssCtx
{
    String(*out);
    int cap;
    int count;
    bool inItem;
    String title;
};

static RssCtx *gCtx = nullptr;

// Keep headlines bounded so one crazy entry doesn't balloon RAM usage
static constexpr int kTitleMax = 160;

// Frees the capacity of an Arduino String
static inline void shrinkString(String &s)
{
    s = "";       // clear contents
    s.reserve(0); // ask core to release the buffer
}

// TinyXML calls this at interesting events.
// nameBuffer is a **path** like "/rss/channel/item/title"
static void xmlCb(uint8_t status, char *nameBuf, uint16_t, char *dataBuf, uint16_t)
{
    if (!gCtx)
        return;
    const String path = String(nameBuf ? nameBuf : "");

    switch (status)
    {
    case STATUS_START_TAG:
        if (path.endsWith("/item"))
        {
            gCtx->inItem = true;
            gCtx->title = "";
            gCtx->title.reserve(96);
            // gCtx->desc = ""; // not used, but leave placeholder
        }
        break;

    case STATUS_TAG_TEXT:
        if (!gCtx->inItem || !dataBuf)
            break;
        if (path.endsWith("/title"))
        {
            // Append but cap length
            int remain = kTitleMax - gCtx->title.length();
            if (remain > 0)
            {
                String chunk(dataBuf);
                if (chunk.length() > remain)
                    chunk.remove(remain);
                gCtx->title += chunk;
            }
        }
        // else if (path.endsWith("/description") || path.endsWith("/content:encoded")) { ... }
        break;

    case STATUS_END_TAG:
        if (path.endsWith("/item") && gCtx->inItem)
        {
            if (gCtx->title.length() && gCtx->count < gCtx->cap)
            {
                String t = htmlDecode(gCtx->title); // reuse your helper
                t.replace("\n", " ");
                t.trim();
                gCtx->out[gCtx->count++] = t;
            }
            // aggressively release per-item temps
            gCtx->title = "";
            shrinkString(gCtx->title);
            // gCtx->desc = "";
            // gCtx->desc.shrinkToFit();
            gCtx->inItem = false;
        }
        break;

    default:
        break;
    }
}

// ---- Public: TinyXML version (capacity = numberOfNewScientistNewsItems) ----

bool fetchNewScientistNews_TinyXML(String out[], int &outCount)
{
    outCount = 0;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    if (!http.begin(client, "https://www.newscientist.com/feed/home/"))
        return false;

    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setTimeout(20000);
    http.setReuse(false);
    http.addHeader("Accept", "application/rss+xml, application/xml;q=0.9,*/*;q=0.8");
    http.addHeader("Accept-Encoding", "identity"); // avoid gzip
    http.addHeader("Connection", "close");
    http.addHeader("User-Agent", "ESP32C3-WeatherDash/1.0");

    int code = http.GET();
    if (code != HTTP_CODE_OK)
    {
        http.end();
        return false;
    }

    // TinyXML working buffer lives on the stack; 512 is a safe size
    uint8_t xmlBuf[512];
    TinyXML parser;
    parser.init(xmlBuf, sizeof(xmlBuf), xmlCb);

    RssCtx ctx{out, numberOfNewScientistNewsItems, 0, false, ""};
    gCtx = &ctx;

    WiFiClient *s = http.getStreamPtr();
    uint8_t chunk[256];
    while (http.connected() || s->available())
    {
        int n = s->read(chunk, sizeof(chunk));
        if (n > 0)
        {
            for (int i = 0; i < n; ++i)
                parser.processChar(chunk[i]);
        }
        else
        {
            delay(1);
        }
    }

    gCtx = nullptr;
    http.end();

    outCount = ctx.count;
    return outCount > 0;
}

bool fetchNewScientistNews(String outItems[numberOfNewScientistNewsItems], int &outCount)
{
    outCount = 0;
    String body;
    if (!fetchNS_XML(body))
    {
        return false;
    }
    outCount = parseRssTitles(body, outItems, numberOfNewScientistNewsItems, "");
    return outCount > 0;
}

void renderNewScientistNewsDashboard(EPaper &ep,
                                     const String items[numberOfNewScientistNewsItems],
                                     int count,
                                     const String &lastUpdatedHHMM, PageRenderMode mode = Full_Page)
{
    renderNewsDashboard(ep, items, count, lastUpdatedHHMM, mode, 1, "New Scientist");
}

// // Public: render (single-column, paginated like ScienceDaily; numbered titles only)
// void renderNewScientistNewsDashboard(EPaper &ep,
//                                      const String items[numberOfNewScientistNewsItems],
//                                      int count,
//                                      const String &lastUpdatedHHMM, PageRenderMode mode = Full_Page)
// {

//     const int HEAD_SZ = 2;
//     const int headH = charH(HEAD_SZ) + 8;
//     ep.setRotation(0);
//     ep.fillScreen(TFT_WHITE);
//     ep.setTextSize(1);
//     String header = F("New Scientist - Latest - Last Updated: ");
//     printHeaderText(ep, header, headH); // rssFeedHandler helper draws title + rule, returns header height
//     ep.update();
//     // Header text (same style/placement as ScienceDaily)
//     String dt;
//     if (lastUpdatedHHMM.length())
//     {
//         dt = lastUpdatedHHMM;
//     }
//     else
//     {
//         struct tm ti{};
//         if (getLocalTime(&ti))
//         {
//             char b1[6];
//             strftime(b1, sizeof(b1), "%H:%M", &ti);
//             char b2[20];
//             strftime(b2, sizeof(b2), "%d %b %Y", &ti);
//             dt = String(b1) + ", " + String(b2);
//         }
//         else
//         {
//             dt = F("--:--, -- --- ----");
//         }
//     }
//     header += dt;
//     // Layout (mirror ScienceDaily): one column, 20 items per page
//     printHeaderText(ep, header, headH); // rssFeedHandler helper draws title + rule, returns header height
//     const int COLS = 1, ROWS = 20;
//     const int colW = 800 / COLS;
//     const int topY = headH + 10;
//     const int itemGap = 3;
//     const int lineGap = 1;

//     int pages = numberOfNewScientistNewsItems / ROWS;
//     for (int p = 0; p < pages; ++p)
//     {
//         if (p > 0)
//         { // clear & re-draw header for each page
//             ep.fillScreen(TFT_WHITE);
//             printHeaderText(ep, header, headH);
//         }

//         int x = 8;
//         int y = topY;

//         for (int r = 0; r < ROWS; ++r)
//         {
//             int idx = p * ROWS + r;
//             if (idx >= count)
//                 break;

//             String num = String(idx + 1) + F(". ");
//             y = printNormalTextLine(ep, num+ items[idx], x, y, colW - 16, lineGap);

//             y += itemGap;
//         }
//         ep.update(); // full-frame refresh
//         if (p < pages - 1)
//         {
//             sleep(30);     // pause before next page
//             deepClean(ep); // optional: deep-clean between pages
//         }
//     }
// }
