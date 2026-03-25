#ifndef UNDO_MANAGER_H
#define UNDO_MANAGER_H

#include <vector>
#include <cstdint>

// A single cell change: position + old/new value.
struct CellDelta {
    int     index;      // flat index into the cells array
    uint8_t oldValue;
    uint8_t newValue;
};

// Delta-based snapshot: stores only the cells that changed,
// plus the grid dimensions before and after the edit.
struct GridDelta {
    int cols = 0;       // grid dimensions BEFORE this edit
    int rows = 0;
    int newCols = 0;    // grid dimensions AFTER this edit
    int newRows = 0;
    std::vector<CellDelta> deltas;
};

// Legacy snapshot returned by undo()/redo() for callers that need
// the full grid state after restoration.
struct GridSnapshot {
    int cols = 0;
    int rows = 0;
    std::vector<uint8_t> cells;
};

// Manages undo/redo history using delta compression.
// Only the cells that actually changed are stored, dramatically reducing
// memory usage for large grids with small edits.
class UndoManager {
public:
    explicit UndoManager(size_t maxHistory = 100);

    // Save the current grid state before an editing operation.
    // Call this BEFORE modifying the grid.
    void saveSnapshot(int cols, int rows, const std::vector<uint8_t>& cells);

    // Finalise the pending edit by computing the delta between the saved
    // pre-edit state and the current (post-edit) state.  If nothing changed
    // the pending snapshot is silently discarded (replaces discardIfUnchanged).
    // Call this AFTER finishing the edit.
    void commitEdit(int cols, int rows, const std::vector<uint8_t>& cells);

    // Legacy helper — equivalent to commitEdit() when cells are unchanged
    // (kept for call-sites that still use the old pattern).
    void discardIfUnchanged(const std::vector<uint8_t>& currentCells);

    // ── Undo / Redo ───────────────────────────────────────────────────────────
    bool canUndo() const;
    bool canRedo() const;

    // Returns a full GridSnapshot after applying the undo/redo delta.
    GridSnapshot undo(int cols, int rows, const std::vector<uint8_t>& currentCells);
    GridSnapshot redo(int cols, int rows, const std::vector<uint8_t>& currentCells);

    // Clear all history (e.g. after loading a new image or resizing)
    void clear();

    // Query
    size_t undoCount() const { return undoStack_.size(); }
    size_t redoCount() const { return redoStack_.size(); }

private:
    size_t maxHistory_;
    std::vector<GridDelta> undoStack_;
    std::vector<GridDelta> redoStack_;

    // Pending pre-edit state (set by saveSnapshot, consumed by commitEdit)
    bool                   hasPending_ = false;
    int                    pendingCols_ = 0;
    int                    pendingRows_ = 0;
    std::vector<uint8_t>   pendingCells_;
};

#endif // UNDO_MANAGER_H
