#ifndef BEAD_GRID_H
#define BEAD_GRID_H

#include <imgui.h>
#include <vector>
#include <cstdint>

// Data model for the perler bead grid.
// Each cell stores a colour index (0 = empty / transparent).
class BeadGrid {
public:
    BeadGrid();

    // Resize the grid. Clears all beads.
    void resize(int cols, int rows);

    int cols() const { return cols_; }
    int rows() const { return rows_; }

    // ── Cell access ───────────────────────────────────────────────────────────
    // colour index: 0 = empty, 1..N = palette entry
    uint8_t get(int col, int row) const;
    void    set(int col, int row, uint8_t colorIdx);

    // Clear the entire grid (set all to 0 = empty)
    void clear();

    // Check if (col, row) is within bounds
    bool inBounds(int col, int row) const;

    // ── Palette ───────────────────────────────────────────────────────────────
    // Index 0 is always "empty" (white / transparent).
    // Indices 1..paletteSize()-1 are the available bead colours.
    static int paletteSize();
    static ImU32 paletteColor(int idx);
    static const char* paletteName(int idx);

private:
    int cols_ = 0;
    int rows_ = 0;
    std::vector<uint8_t> cells_; // row-major: cells_[row * cols_ + col]
};

#endif // BEAD_GRID_H
