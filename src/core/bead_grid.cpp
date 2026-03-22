#include "core/bead_grid.h"
#include <algorithm>

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
