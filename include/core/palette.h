#ifndef PALETTE_H
#define PALETTE_H

#include <imgui.h>
#include <string>
#include <vector>

// A single palette entry: a named colour.
struct PaletteEntry {
    std::string name;
    ImU32       color; // packed RGBA via IM_COL32
};

// Manages the set of available bead colours.
// Index 0 is always "Empty" (white). Indices 1..size()-1 are bead colours.
// Loaded from a JSON file; falls back to a built-in default if the file is
// missing or malformed.
class Palette {
public:
    Palette();

    // Load palette entries from a JSON file.
    // On failure, logs a warning and keeps the current (default) palette.
    void loadFromFile(const std::string& path);

    // Number of entries (including index 0 = Empty)
    int size() const { return static_cast<int>(entries_.size()); }

    // Access by index (out-of-range returns entry 0)
    ImU32       color(int idx) const;
    const char* name(int idx)  const;

    // Find the palette index whose colour is closest to (r, g, b).
    // Skips index 0 (Empty). Returns 1..size()-1.
    int matchNearest(int r, int g, int b) const;

private:
    std::vector<PaletteEntry> entries_;

    // Populate with a sensible built-in default
    void loadDefaults();
};

#endif // PALETTE_H
