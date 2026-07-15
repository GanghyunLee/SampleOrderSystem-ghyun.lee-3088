#pragma once

// Minimal recursive-descent JSON parser. Failures are reported via a bool
// return value (TryParse) rather than exceptions, per docs/architecture.md §2.

#include <string>

#include "JsonValue.h"

class JsonParser {
public:
    // Parses text and returns the resulting value, or a Null JsonValue if
    // parsing fails. Use TryParse if you need to distinguish "parsed to null"
    // from "failed to parse".
    static JsonValue Parse(const std::string& text);

    // Attempts to parse text into out. Returns false (without throwing) on
    // malformed JSON.
    static bool TryParse(const std::string& text, JsonValue& out);
};
