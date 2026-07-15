#include "SaveManager.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "JsonParser.h"

bool SaveManager::Save(const std::string& filePath, const JsonValue& data) {
    std::filesystem::path path(filePath);
    std::filesystem::path parent = path.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent)) {
        try {
            std::filesystem::create_directories(parent);
        } catch (const std::filesystem::filesystem_error&) {
            return false;
        }
    }

    std::ofstream file(filePath, std::ios::trunc | std::ios::binary);
    if (!file.is_open()) return false;
    file << data.Serialize();
    return file.good();
}

bool SaveManager::Load(const std::string& filePath, JsonValue& out) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return false;
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return JsonParser::TryParse(buffer.str(), out);
}
