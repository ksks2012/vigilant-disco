#include "app/config.h"
#include "logging/logger.h"

#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

void Config::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw ConfigError("Cannot open config file: " + path);
    }

    json j;
    try {
        file >> j;
    } catch (const json::parse_error& e) {
        throw ConfigError("JSON parse error: " + std::string(e.what()));
    }

    // Logger
    if (j.contains("logger_level")) {
        logger_level = j["logger_level"].get<int>();
    }

    // Window
    if (j.contains("window")) {
        auto& w = j["window"];
        if (w.contains("width"))  window.width  = w["width"].get<int>();
        if (w.contains("height")) window.height = w["height"].get<int>();
        if (w.contains("title"))  window.title  = w["title"].get<std::string>();
    }

    LOG_INFO("Config", "Loaded configuration from " + path);
}
