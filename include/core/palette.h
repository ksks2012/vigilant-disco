#ifndef PALETTE_H
#define PALETTE_H

#include <imgui.h>
#include <string>
#include <vector>

// A single palette entry: a named colour with an optional product code.
struct PaletteEntry {
    std::string name;
    std::string code;   // manufacturer code, e.g. "H01", "P05"
    ImU32       color;  // packed RGBA via IM_COL32
};

// Colour-matching algorithm for palette look-up.
enum class ColorMatchMethod {
    EuclideanRGB,   // Standard squared Euclidean distance in RGB
    Redmean         // Perceptually weighted: (2+r̄/256)·ΔR² + 4·ΔG² + (2+(255-r̄)/256)·ΔB²
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
    const char* code(int idx)  const;

    // Find the palette index whose colour is closest to (r, g, b).
    // Skips index 0 (Empty). Returns 1..size()-1.
    int matchNearest(int r, int g, int b,
                     ColorMatchMethod method = ColorMatchMethod::EuclideanRGB) const;

    // Direct access to entries (for serialisation)
    const std::vector<PaletteEntry>& entries() const { return entries_; }

    // Replace palette entries (used when loading a project file)
    void setEntries(std::vector<PaletteEntry> entries) { entries_ = std::move(entries); }

private:
    std::vector<PaletteEntry> entries_;

    // Populate with a sensible built-in default
    void loadDefaults();
};

#endif // PALETTE_H
