#pragma once
#include <Arduino.h>

/**
 * Decode HTML/Atom entities in a string.
 *
 * - Handles common named entities (&amp;, &nbsp;, &ndash;, &hellip;, quotes…)
 * - Handles numeric entities (&#169; and &#x2014;)
 * - asciiFallback=true maps punctuation to safe ASCII for bitmap fonts
 *
 * Example:
 *   String t = htmlDecode("A &amp; B &ndash; C", /*asciiFallback=*\/true);
 */
String htmlDecode(const String &s, bool asciiFallback = false);
