#include "core/bead_grid.h"
#include <algorithm>
#include <cstring>

// ── Palette definition ────────────────────────────────────────────────────────
// A small starter palette for basic editing. Index 0 = empty.
struct PaletteEntry {
    const char* name;
    ImU32       color;
};

static const PaletteEntry kPalette[] = {
    { "Empty",       IM_COL32(255, 255, 255, 255) },  // 0 – white / empty
    { "Black",       IM_COL32(  0,   0,   0, 255) },  // 1
    { "White",       IM_COL32(245, 245, 245, 255) },  // 2
    { "Red",         IM_COL32(220,  40,  40, 255) },  // 3
    { "Orange",      IM_COL32(240, 150,  30, 255) },  // 4
    { "Yellow",      IM_COL32(250, 220,  40, 255) },  // 5
    { "Green",       IM_COL32( 50, 180,  60, 255) },  // 6
    { "Blue",        IM_COL32( 40, 100, 220, 255) },  // 7
    { "Purple",      IM_COL32(140,  60, 180, 255) },  // 8
    { "Pink",        IM_COL32(240, 130, 170, 255) },  // 9
    { "Brown",       IM_COL32(140,  90,  50, 255) },  // 10
    { "Light Grey",  IM_COL32(190, 190, 190, 255) },  // 11
    { "Dark Grey",   IM_COL32(100, 100, 100, 255) },  // 12
};

static constexpr int kPaletteCount = static_cast<int>(sizeof(kPalette) / sizeof(kPalette[0]));

// ── BeadGrid implementation ───────────────────────────────────────────────────

BeadGrid::BeadGrid() = default;

void BeadGrid::resize(int cols, int rows) {
    cols_ = std::max(1, cols);
    rows_ = std::max(1, rows);
    cells_.assign(static_cast<size_t>(cols_) * rows_, 0);
}

uint8_t BeadGrid::get(int col, int row) const {
    if (!inBounds(col, row)) return 0;
    return cells_[static_cast<size_t>(row) * cols_ + col];
}

void BeadGrid::set(int col, int row, uint8_t colorIdx) {
    if (!inBounds(col, row)) return;
    cells_[static_cast<size_t>(row) * cols_ + col] = colorIdx;
}

void BeadGrid::clear() {
    std::fill(cells_.begin(), cells_.end(), static_cast<uint8_t>(0));
}

bool BeadGrid::inBounds(int col, int row) const {
    return col >= 0 && col < cols_ && row >= 0 && row < rows_;
}

int BeadGrid::paletteSize() {
    return kPaletteCount;
}

ImU32 BeadGrid::paletteColor(int idx) {
    if (idx < 0 || idx >= kPaletteCount) return kPalette[0].color;
    return kPalette[idx].color;
}

const char* BeadGrid::paletteName(int idx) {
    if (idx < 0 || idx >= kPaletteCount) return kPalette[0].name;
    return kPalette[idx].name;
}
