#ifndef PROGRESS_TRACKER_H
#define PROGRESS_TRACKER_H

#include <vector>
#include <cstdint>

// Tracks which beads have been physically placed ("completed") during
// a real-world assembly session.  Each cell stores 0 (pending) or 1 (done).
// The tracker is grid-size-aware and automatically resizes when the grid
// dimensions change.
class ProgressTracker {
public:
    ProgressTracker() = default;

    // Resize the progress grid (clears all marks).
    void resize(int cols, int rows);

    int cols() const { return cols_; }
    int rows() const { return rows_; }

    // ── Cell access ───────────────────────────────────────────────────────────
    bool isDone(int col, int row) const;
    void setDone(int col, int row, bool done);
    void toggle(int col, int row);

    // Convenience for brush-sized marking (same semantics as BeadGrid::paintBrush)
    void markBrush(int col, int row, bool done, int brushSize);

    // ── Bulk operations ───────────────────────────────────────────────────────
    void clearAll();                // reset everything to pending
    void markAll();                 // mark everything as done

    // ── Statistics ────────────────────────────────────────────────────────────
    int  totalCells()  const { return static_cast<int>(cells_.size()); }
    int  doneCount()   const;
    // Count done cells within a rectangular sub-region [startCol, endCol) x [startRow, endRow)
    int  doneCountInRect(int startCol, int startRow, int endCol, int endRow) const;

    // ── Serialisation helpers ─────────────────────────────────────────────────
    const std::vector<uint8_t>& cells() const { return cells_; }
    void restoreFrom(int cols, int rows, const std::vector<uint8_t>& cells);

    bool inBounds(int col, int row) const;

private:
    int cols_ = 0;
    int rows_ = 0;
    std::vector<uint8_t> cells_;   // row-major, 0 = pending, 1 = done
};

#endif // PROGRESS_TRACKER_H
