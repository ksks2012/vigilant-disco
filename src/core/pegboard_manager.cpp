#include "core/pegboard_manager.h"

#include <algorithm>
#include <cmath>

// ── Layout recalculation ──────────────────────────────────────────────────────

void PegboardManager::update(int gridCols, int gridRows, int pegboardSize) {
    pegSize_ = std::max(1, pegboardSize);
    tilesX_  = (gridCols + pegSize_ - 1) / pegSize_;  // ceil division
    tilesY_  = (gridRows + pegSize_ - 1) / pegSize_;

    tiles_.clear();
    tiles_.reserve(static_cast<size_t>(tilesX_) * tilesY_);

    int id = 0;
    for (int ty = 0; ty < tilesY_; ++ty) {
        for (int tx = 0; tx < tilesX_; ++tx) {
            PegboardInfo info{};
            info.id      = id++;
            info.gridCol = tx;
            info.gridRow = ty;
            info.startCol = tx * pegSize_;
            info.startRow = ty * pegSize_;
            info.endCol   = std::min(info.startCol + pegSize_, gridCols);
            info.endRow   = std::min(info.startRow + pegSize_, gridRows);
            tiles_.push_back(info);
        }
    }
}

// ── Tile access ───────────────────────────────────────────────────────────────

const PegboardInfo& PegboardManager::tile(int index) const {
    static const PegboardInfo empty{};
    if (index < 0 || index >= static_cast<int>(tiles_.size())) return empty;
    return tiles_[index];
}

int PegboardManager::tileAt(int col, int row) const {
    if (pegSize_ <= 0) return -1;
    int tx = col / pegSize_;
    int ty = row / pegSize_;
    if (tx < 0 || tx >= tilesX_ || ty < 0 || ty >= tilesY_) return -1;
    return ty * tilesX_ + tx;
}

// ── Label generation ──────────────────────────────────────────────────────────

std::string PegboardManager::tileLabel(const PegboardInfo& info) {
    // Row letter A-Z (wraps for very large grids), column number 1-based
    char rowChar = static_cast<char>('A' + (info.gridRow % 26));
    return std::string("Board ") + rowChar + std::to_string(info.gridCol + 1);
}

// ── Bead counting ─────────────────────────────────────────────────────────────

int PegboardManager::countBeads(const PegboardInfo& info, const BeadGrid& grid) {
    int count = 0;
    for (int r = info.startRow; r < info.endRow; ++r) {
        for (int c = info.startCol; c < info.endCol; ++c) {
            if (grid.get(c, r) != 0) ++count;
        }
    }
    return count;
}

std::vector<int> PegboardManager::countBeadsPerColour(const PegboardInfo& info,
                                                       const BeadGrid& grid,
                                                       int paletteSize) {
    std::vector<int> counts(paletteSize, 0);
    for (int r = info.startRow; r < info.endRow; ++r) {
        for (int c = info.startCol; c < info.endCol; ++c) {
            uint8_t idx = grid.get(c, r);
            if (idx > 0 && idx < paletteSize) {
                counts[idx]++;
            }
        }
    }
    return counts;
}
