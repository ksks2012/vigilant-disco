#include "core/project_file.h"
#include "logging/logger.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

static constexpr int kFileVersion = 1;

// ── Save ──────────────────────────────────────────────────────────────────────

ProjectFileResult ProjectFile::save(const std::string& path,
                                    const BeadGrid& grid,
                                    const Palette& palette) {
    ProjectFileResult result;

    json j;
    j["version"] = kFileVersion;

    // ── Grid ──────────────────────────────────────────────────────────────────
    json jGrid;
    jGrid["cols"] = grid.cols();
    jGrid["rows"] = grid.rows();

    // Store cells as a flat array of colour indices
    const auto& cells = grid.cells();
    jGrid["cells"] = json::array();
    for (uint8_t c : cells) {
        jGrid["cells"].push_back(static_cast<int>(c));
    }
    j["grid"] = jGrid;

    // ── Palette (skip index 0 = Empty, which is implicit) ─────────────────────
    json jPalette = json::array();
    const auto& entries = palette.entries();
    for (int i = 1; i < static_cast<int>(entries.size()); ++i) {
        const auto& e = entries[i];
        int r = static_cast<int>((e.color >> IM_COL32_R_SHIFT) & 0xFF);
        int g = static_cast<int>((e.color >> IM_COL32_G_SHIFT) & 0xFF);
        int b = static_cast<int>((e.color >> IM_COL32_B_SHIFT) & 0xFF);
        json je = { {"name", e.name}, {"color", {r, g, b}} };
        if (!e.code.empty()) {
            je["code"] = e.code;
        }
        jPalette.push_back(je);
    }
    j["palette"] = jPalette;

    // ── Write to file ─────────────────────────────────────────────────────────
    std::ofstream file(path);
    if (!file.is_open()) {
        result.message = "Cannot open file for writing: " + path;
        LOG_ERROR("ProjectFile", result.message);
        return result;
    }

    try {
        file << j.dump(2); // pretty-print with 2-space indent
    } catch (const std::exception& e) {
        result.message = std::string("Write error: ") + e.what();
        LOG_ERROR("ProjectFile", result.message);
        return result;
    }

    result.success = true;
    result.message = "Saved to " + path;
    LOG_INFO("ProjectFile", result.message);
    return result;
}

// ── Load ──────────────────────────────────────────────────────────────────────

ProjectFileResult ProjectFile::load(const std::string& path,
                                    BeadGrid& grid,
                                    Palette& palette) {
    ProjectFileResult result;

    std::ifstream file(path);
    if (!file.is_open()) {
        result.message = "Cannot open file: " + path;
        LOG_ERROR("ProjectFile", result.message);
        return result;
    }

    json j;
    try {
        file >> j;
    } catch (const json::parse_error& e) {
        result.message = std::string("JSON parse error: ") + e.what();
        LOG_ERROR("ProjectFile", result.message);
        return result;
    }

    // ── Version check ─────────────────────────────────────────────────────────
    int version = j.value("version", 0);
    if (version < 1) {
        result.message = "Unknown or missing file version";
        LOG_ERROR("ProjectFile", result.message);
        return result;
    }

    // ── Palette ───────────────────────────────────────────────────────────────
    if (!j.contains("palette") || !j["palette"].is_array()) {
        result.message = "Missing 'palette' array in project file";
        LOG_ERROR("ProjectFile", result.message);
        return result;
    }

    std::vector<PaletteEntry> entries;
    entries.push_back({ "Empty", "", IM_COL32(255, 255, 255, 255) }); // index 0

    for (const auto& entry : j["palette"]) {
        if (!entry.contains("name") || !entry.contains("color")) continue;

        std::string name = entry["name"].get<std::string>();
        std::string code;
        if (entry.contains("code") && entry["code"].is_string()) {
            code = entry["code"].get<std::string>();
        }
        if (!entry["color"].is_array() || entry["color"].size() < 3) continue;

        int r = std::clamp(entry["color"][0].get<int>(), 0, 255);
        int g = std::clamp(entry["color"][1].get<int>(), 0, 255);
        int b = std::clamp(entry["color"][2].get<int>(), 0, 255);
        entries.push_back({ name, code, IM_COL32(r, g, b, 255) });
    }

    if (entries.size() <= 1) {
        result.message = "Palette in project file has no valid entries";
        LOG_ERROR("ProjectFile", result.message);
        return result;
    }

    // ── Grid ──────────────────────────────────────────────────────────────────
    if (!j.contains("grid") || !j["grid"].is_object()) {
        result.message = "Missing 'grid' object in project file";
        LOG_ERROR("ProjectFile", result.message);
        return result;
    }

    const auto& jGrid = j["grid"];
    int cols = jGrid.value("cols", 0);
    int rows = jGrid.value("rows", 0);
    if (cols < 1 || rows < 1) {
        result.message = "Invalid grid dimensions: " +
                         std::to_string(cols) + "x" + std::to_string(rows);
        LOG_ERROR("ProjectFile", result.message);
        return result;
    }

    if (!jGrid.contains("cells") || !jGrid["cells"].is_array()) {
        result.message = "Missing 'cells' array in grid";
        LOG_ERROR("ProjectFile", result.message);
        return result;
    }

    const auto& jCells = jGrid["cells"];
    size_t expectedSize = static_cast<size_t>(cols) * rows;
    if (jCells.size() != expectedSize) {
        result.message = "Cell count mismatch: expected " +
                         std::to_string(expectedSize) + ", got " +
                         std::to_string(jCells.size());
        LOG_ERROR("ProjectFile", result.message);
        return result;
    }

    // Validate and clamp cell values to palette range
    int paletteMax = static_cast<int>(entries.size()) - 1;
    std::vector<uint8_t> cells(expectedSize);
    for (size_t i = 0; i < expectedSize; ++i) {
        int v = jCells[i].get<int>();
        cells[i] = static_cast<uint8_t>(std::clamp(v, 0, paletteMax));
    }

    // ── Apply ─────────────────────────────────────────────────────────────────
    palette.setEntries(std::move(entries));
    grid.restoreFrom(cols, rows, cells);

    result.success = true;
    result.message = "Loaded " + std::to_string(cols) + "x" +
                     std::to_string(rows) + " grid from " + path;
    LOG_INFO("ProjectFile", result.message);
    return result;
}
