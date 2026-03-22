#include "rendering/grid_renderer.h"
#include <cmath>
#include <algorithm>

GridRenderer::GridRenderer() = default;

void GridRenderer::draw(Canvas& canvas, const BeadGrid& grid) const {
    ImDrawList* dl = canvas.drawList();
    if (!dl) return;

    int cols = grid.cols();
    int rows = grid.rows();
    float scale = canvas.scale();

    // ── Determine visible range to avoid drawing off-screen beads ─────────────
    ImVec2 topLeftWorld  = canvas.screenToWorld(canvas.origin().x, canvas.origin().y);
    ImVec2 botRightWorld = canvas.screenToWorld(
        canvas.origin().x + canvas.size().x,
        canvas.origin().y + canvas.size().y);

    // Bead centres are at (col + 0.5, row + 0.5) in world units
    int minCol = std::max(0, static_cast<int>(std::floor(topLeftWorld.x - 0.5f)));
    int maxCol = std::min(cols - 1, static_cast<int>(std::ceil(botRightWorld.x - 0.5f)));
    int minRow = std::max(0, static_cast<int>(std::floor(topLeftWorld.y - 0.5f)));
    int maxRow = std::min(rows - 1, static_cast<int>(std::ceil(botRightWorld.y - 0.5f)));

    if (minCol > maxCol || minRow > maxRow) return;

    // ── Draw board background ─────────────────────────────────────────────────
    ImVec2 boardTL = canvas.worldToScreen(0.0f, 0.0f);
    ImVec2 boardBR = canvas.worldToScreen(static_cast<float>(cols),
                                          static_cast<float>(rows));
    dl->AddRectFilled(boardTL, boardBR, kBoardColor, 4.0f);

    // ── Draw beads ────────────────────────────────────────────────────────────
    float beadRadiusPx   = kBeadRadius * scale;
    float outlineRadiusPx = kOutlineRadius * scale;

    // Adaptive detail: fewer segments when zoomed out
    int segments = 0; // 0 = auto
    if (beadRadiusPx < 4.0f) segments = 8;
    else if (beadRadiusPx < 8.0f) segments = 12;

    // Skip bead drawing entirely if they would be sub-pixel
    bool drawBeads = (beadRadiusPx >= 1.5f);

    for (int row = minRow; row <= maxRow; ++row) {
        for (int col = minCol; col <= maxCol; ++col) {
            // Bead centre in world units
            float cx = static_cast<float>(col) + 0.5f;
            float cy = static_cast<float>(row) + 0.5f;
            ImVec2 centre = canvas.worldToScreen(cx, cy);

            // Look up bead colour from the data model
            uint8_t colorIdx = grid.get(col, row);
            ImU32 fillColor = BeadGrid::paletteColor(colorIdx);

            if (drawBeads) {
                // Outline circle
                dl->AddCircleFilled(centre, outlineRadiusPx, kOutlineColor, segments);
                // Inner bead
                dl->AddCircleFilled(centre, beadRadiusPx, fillColor, segments);
                // Small centre peg (only show on empty beads when zoomed in)
                if (colorIdx == 0 && beadRadiusPx > 5.0f) {
                    float pegR = std::max(1.5f, beadRadiusPx * 0.15f);
                    dl->AddCircleFilled(centre, pegR, kPegColor, segments);
                }
            } else {
                // Zoomed way out: draw simple rectangles
                float halfPx = scale * 0.45f;
                dl->AddRectFilled(
                    ImVec2(centre.x - halfPx, centre.y - halfPx),
                    ImVec2(centre.x + halfPx, centre.y + halfPx),
                    fillColor);
            }
        }
    }
}
