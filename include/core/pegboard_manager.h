#ifndef PEGBOARD_MANAGER_H
#define PEGBOARD_MANAGER_H

#include "core/bead_grid.h"
#include "core/palette.h"

#include <vector>
#include <string>

// Information about one pegboard tile within the overall grid.
struct PegboardInfo {
    int id;            // 0-based sequential index (row-major)
    int gridCol;       // column index in the tile grid (0-based)
    int gridRow;       // row index in the tile grid (0-based)

    // Cell range in the bead grid (inclusive start, exclusive end)
    int startCol;
    int startRow;
    int endCol;        // exclusive
    int endRow;        // exclusive

    // Actual tile size (may be smaller at right/bottom edges)
    int tileCols() const { return endCol - startCol; }
    int tileRows() const { return endRow - startRow; }
};

// Manages the subdivision of a large bead grid into standard pegboard tiles.
class PegboardManager {
public:
    // Recalculate tile layout for the given grid dimensions and pegboard size.
    void update(int gridCols, int gridRows, int pegboardSize);

    // ── Queries ───────────────────────────────────────────────────────────────
    int tileCountX() const { return tilesX_; }
    int tileCountY() const { return tilesY_; }
    int totalTiles() const { return static_cast<int>(tiles_.size()); }
    int pegboardSize() const { return pegSize_; }

    // Access a specific tile by its sequential index.
    const PegboardInfo& tile(int index) const;

    // All tiles.
    const std::vector<PegboardInfo>& tiles() const { return tiles_; }

    // Return the tile index that contains a given cell, or -1 if out of range.
    int tileAt(int col, int row) const;

    // Display label for a tile, e.g. "Board A1", "Board B3".
    static std::string tileLabel(const PegboardInfo& info);

    // Count non-empty beads within a tile.
    static int countBeads(const PegboardInfo& info, const BeadGrid& grid);

    // Count beads per colour within a tile.  Returns a vector indexed by
    // palette index.  Size is paletteSize.
    static std::vector<int> countBeadsPerColour(const PegboardInfo& info,
                                                 const BeadGrid& grid,
                                                 int paletteSize);

private:
    int pegSize_ = 29;
    int tilesX_  = 1;
    int tilesY_  = 1;
    std::vector<PegboardInfo> tiles_;
};

#endif // PEGBOARD_MANAGER_H
