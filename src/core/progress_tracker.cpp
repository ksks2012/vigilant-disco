#include "core/progress_tracker.h"

#include <algorithm>

// ── Resize ────────────────────────────────────────────────────────────────────

void ProgressTracker::resize(int cols, int rows) {
    cols_ = cols;
    rows_ = rows;
    cells_.assign(static_cast<size_t>(cols) * rows, 0);
}

// ── Cell access ───────────────────────────────────────────────────────────────

bool ProgressTracker::inBounds(int col, int row) const {
    return col >= 0 && col < cols_ && row >= 0 && row < rows_;
}

bool ProgressTracker::isDone(int col, int row) const {
    if (!inBounds(col, row)) return false;
    return cells_[static_cast<size_t>(row * cols_ + col)] != 0;
}

void ProgressTracker::setDone(int col, int row, bool done) {
    if (!inBounds(col, row)) return;
    cells_[static_cast<size_t>(row * cols_ + col)] = done ? 1 : 0;
}

void ProgressTracker::toggle(int col, int row) {
    if (!inBounds(col, row)) return;
    auto& c = cells_[static_cast<size_t>(row * cols_ + col)];
    c = (c != 0) ? 0 : 1;
}

void ProgressTracker::markBrush(int col, int row, bool done, int brushSize) {
    int half = brushSize - 1; // 1→0, 2→1, 3→2
    for (int dy = -half; dy <= half; ++dy) {
        for (int dx = -half; dx <= half; ++dx) {
            setDone(col + dx, row + dy, done);
        }
    }
}

// ── Bulk operations ───────────────────────────────────────────────────────────

void ProgressTracker::clearAll() {
    std::fill(cells_.begin(), cells_.end(), static_cast<uint8_t>(0));
}

void ProgressTracker::markAll() {
    std::fill(cells_.begin(), cells_.end(), static_cast<uint8_t>(1));
}

// ── Statistics ────────────────────────────────────────────────────────────────

int ProgressTracker::doneCount() const {
    int count = 0;
    for (uint8_t c : cells_) {
        if (c != 0) ++count;
    }
    return count;
}

int ProgressTracker::doneCountInRect(int startCol, int startRow,
                                      int endCol, int endRow) const {
    int count = 0;
    for (int r = startRow; r < endRow && r < rows_; ++r) {
        if (r < 0) continue;
        for (int c = startCol; c < endCol && c < cols_; ++c) {
            if (c < 0) continue;
            if (cells_[static_cast<size_t>(r * cols_ + c)] != 0) ++count;
        }
    }
    return count;
}

// ── Serialisation ─────────────────────────────────────────────────────────────

void ProgressTracker::restoreFrom(int cols, int rows,
                                   const std::vector<uint8_t>& cells) {
    cols_ = cols;
    rows_ = rows;
    cells_ = cells;
    // Ensure correct size
    cells_.resize(static_cast<size_t>(cols) * rows, 0);
}
