#ifndef BEAD_GRID_H
#define BEAD_GRID_H

#include <imgui.h>
#include <vector>
#include <cstdint>

#include "core/palette.h"

// Available editing tools
enum class Tool {
    Brush,       // Paint individual beads (size 1–3)
    Eyedropper,  // Pick colour from the grid
    FloodFill    // Fill connected region with the selected colour
};

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

    // Paint a square brush of the given radius (1 = single bead, 2 = 3x3, 3 = 5x5)
    void paintBrush(int col, int row, uint8_t colorIdx, int brushSize);

    // Flood-fill: replace all connected cells of the same colour starting at
    // (col, row) with newColorIdx. Uses iterative BFS to avoid stack overflow.
    void floodFill(int col, int row, uint8_t newColorIdx);

    // Remove background by flood-filling from all four edges.
    // Any connected cell whose palette colour is within `tolerance` (squared
    // Euclidean RGB distance) of the edge cell's colour is set to Empty (0).
    // Returns the number of cells cleared.
    int removeBackground(const Palette& palette, int tolerance);

    // Clear the entire grid (set all to 0 = empty)
    void clear();

    // Check if (col, row) is within bounds
    bool inBounds(int col, int row) const;

    // ── Snapshot support (for undo/redo) ──────────────────────────────────────
    const std::vector<uint8_t>& cells() const { return cells_; }

    // Restore grid state from a snapshot
    void restoreFrom(int cols, int rows, const std::vector<uint8_t>& cells);

    // ── Palette ───────────────────────────────────────────────────────────────
    void           setPalette(Palette* palette) { palette_ = palette; }
    const Palette* palette() const { return palette_; }

private:
    int cols_ = 0;
    int rows_ = 0;
    std::vector<uint8_t> cells_; // row-major: cells_[row * cols_ + col]
    Palette* palette_ = nullptr;
};

#endif // BEAD_GRID_H
