#pragma once

// File-based persistence for JsonValue trees, per docs/architecture.md
// section 6. Failures (missing file, unwritable path) are reported via
// bool return values rather than exceptions.

#include <string>

#include "JsonValue.h"

class SaveManager {
public:
    static bool Save(const std::string& filePath, const JsonValue& data);
    static bool Load(const std::string& filePath, JsonValue& out);
};
