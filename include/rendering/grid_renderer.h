#ifndef GRID_RENDERER_H
#define GRID_RENDERER_H

#include "app/canvas.h"
#include <vector>
#include <cstdint>

// Renders a perler bead grid on an ImGui Canvas.
// Each cell is drawn as a filled circle (bead) with an outline.
class GridRenderer {
public:
    GridRenderer();

    // Set grid dimensions (number of beads)
    void resize(int cols, int rows);

    int cols() const { return cols_; }
    int rows() const { return rows_; }

    // Draw the grid onto the given canvas
    void draw(Canvas& canvas) const;

private:
    int cols_ = 29;
    int rows_ = 29;

    // Appearance constants (in world units where 1 unit = 1 bead pitch)
    static constexpr float kBeadRadius    = 0.40f;  // slightly less than 0.5 to show gaps
    static constexpr float kOutlineRadius = 0.44f;

    // Colours
    static constexpr ImU32 kEmptyFill    = IM_COL32(255, 255, 255, 255);
    static constexpr ImU32 kOutlineColor = IM_COL32(200, 200, 200, 255);
    static constexpr ImU32 kBoardColor   = IM_COL32(230, 230, 235, 255);
    static constexpr ImU32 kPegColor     = IM_COL32(180, 180, 185, 255);
};

#endif // GRID_RENDERER_H
