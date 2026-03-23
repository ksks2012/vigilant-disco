#include "core/bead_grid.h"
#include <algorithm>
#include <queue>

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

void BeadGrid::restoreFrom(int cols, int rows, const std::vector<uint8_t>& cells) {
    cols_ = cols;
    rows_ = rows;
    cells_ = cells;
}

void BeadGrid::paintBrush(int col, int row, uint8_t colorIdx, int brushSize) {
    // brushSize 1 = single bead, 2 = 3x3, 3 = 5x5
    int radius = brushSize - 1; // 0, 1, or 2
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            set(col + dx, row + dy, colorIdx);
        }
    }
}

void BeadGrid::floodFill(int col, int row, uint8_t newColorIdx) {
    if (!inBounds(col, row)) return;

    uint8_t targetColor = get(col, row);
    if (targetColor == newColorIdx) return; // already the desired colour

    // Iterative BFS to avoid stack overflow on large grids
    std::queue<std::pair<int, int>> q;
    q.push({ col, row });
    set(col, row, newColorIdx);

    // 4-connected neighbours (up, down, left, right)
    static constexpr int dx[] = { 0, 0, -1, 1 };
    static constexpr int dy[] = { -1, 1, 0, 0 };

    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();

        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];
            if (inBounds(nx, ny) && get(nx, ny) == targetColor) {
                set(nx, ny, newColorIdx);
                q.push({ nx, ny });
            }
        }
    }
}
