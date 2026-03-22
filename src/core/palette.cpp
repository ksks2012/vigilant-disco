#include "core/palette.h"
#include "logging/logger.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

// ── Built-in default palette ──────────────────────────────────────────────────

void Palette::loadDefaults() {
    entries_.clear();
    entries_.push_back({ "Empty",       IM_COL32(255, 255, 255, 255) });
    entries_.push_back({ "Black",       IM_COL32(  0,   0,   0, 255) });
    entries_.push_back({ "White",       IM_COL32(245, 245, 245, 255) });
    entries_.push_back({ "Red",         IM_COL32(220,  40,  40, 255) });
    entries_.push_back({ "Orange",      IM_COL32(240, 150,  30, 255) });
    entries_.push_back({ "Yellow",      IM_COL32(250, 220,  40, 255) });
    entries_.push_back({ "Green",       IM_COL32( 50, 180,  60, 255) });
    entries_.push_back({ "Blue",        IM_COL32( 40, 100, 220, 255) });
    entries_.push_back({ "Purple",      IM_COL32(140,  60, 180, 255) });
    entries_.push_back({ "Pink",        IM_COL32(240, 130, 170, 255) });
    entries_.push_back({ "Brown",       IM_COL32(140,  90,  50, 255) });
    entries_.push_back({ "Light Grey",  IM_COL32(190, 190, 190, 255) });
    entries_.push_back({ "Dark Grey",   IM_COL32(100, 100, 100, 255) });
}

Palette::Palette() {
    loadDefaults();
}

// ── JSON loading ──────────────────────────────────────────────────────────────

void Palette::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_WARN("Palette", "Cannot open palette file: " + path + " (using defaults)");
        return;
    }

    json j;
    try {
        file >> j;
    } catch (const json::parse_error& e) {
        LOG_WARN("Palette", "JSON parse error in " + path + ": " + e.what() + " (using defaults)");
        return;
    }

    if (!j.contains("palette") || !j["palette"].is_array()) {
        LOG_WARN("Palette", "No 'palette' array found in " + path + " (using defaults)");
        return;
    }

    std::vector<PaletteEntry> loaded;

    // Index 0 is always Empty (not overridable)
    loaded.push_back({ "Empty", IM_COL32(255, 255, 255, 255) });

    for (const auto& entry : j["palette"]) {
        if (!entry.contains("name") || !entry.contains("color")) {
            LOG_WARN("Palette", "Skipping palette entry: missing 'name' or 'color'");
            continue;
        }

        std::string name = entry["name"].get<std::string>();

        // Parse colour: expect [R, G, B] array with values 0-255
        if (!entry["color"].is_array() || entry["color"].size() < 3) {
            LOG_WARN("Palette", "Skipping '" + name + "': 'color' must be an [R,G,B] array");
            continue;
        }

        int r = std::clamp(entry["color"][0].get<int>(), 0, 255);
        int g = std::clamp(entry["color"][1].get<int>(), 0, 255);
        int b = std::clamp(entry["color"][2].get<int>(), 0, 255);

        loaded.push_back({ name, IM_COL32(r, g, b, 255) });
    }

    if (loaded.size() <= 1) {
        LOG_WARN("Palette", "Palette file had no valid entries (using defaults)");
        return;
    }

    entries_ = std::move(loaded);
    LOG_INFO("Palette", "Loaded " + std::to_string(entries_.size() - 1) +
             " colours from " + path);
}

// ── Access ────────────────────────────────────────────────────────────────────

ImU32 Palette::color(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(entries_.size())) return entries_[0].color;
    return entries_[idx].color;
}

const char* Palette::name(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(entries_.size())) return entries_[0].name.c_str();
    return entries_[idx].name.c_str();
}
