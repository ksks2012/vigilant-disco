#ifndef UNDO_MANAGER_H
#define UNDO_MANAGER_H

#include <vector>
#include <cstdint>

// Snapshot of the grid state for undo/redo.
struct GridSnapshot {
    int cols = 0;
    int rows = 0;
    std::vector<uint8_t> cells;
};

// Manages undo/redo history using full-grid snapshots.
// Typical grid sizes (up to 256x256 = 64 KB per snapshot) make this
// approach simple and efficient enough.
class UndoManager {
public:
    explicit UndoManager(size_t maxHistory = 100);

    // Save the current grid state before an editing operation.
    // Call this BEFORE modifying the grid.
    void saveSnapshot(int cols, int rows, const std::vector<uint8_t>& cells);

    // Discard the most recent snapshot if it is identical to the current state.
    // Call this AFTER an operation that might not have changed anything
    // (e.g. painting the same colour over itself).
    void discardIfUnchanged(const std::vector<uint8_t>& currentCells);

    // ── Undo / Redo ───────────────────────────────────────────────────────────
    bool canUndo() const;
    bool canRedo() const;

    // Returns the snapshot to restore. The caller is responsible for
    // applying it to the grid.
    GridSnapshot undo(int cols, int rows, const std::vector<uint8_t>& currentCells);
    GridSnapshot redo(int cols, int rows, const std::vector<uint8_t>& currentCells);

    // Clear all history (e.g. after loading a new image or resizing)
    void clear();

    // Query
    size_t undoCount() const { return undoStack_.size(); }
    size_t redoCount() const { return redoStack_.size(); }

private:
    size_t maxHistory_;
    std::vector<GridSnapshot> undoStack_;
    std::vector<GridSnapshot> redoStack_;
};

#endif // UNDO_MANAGER_H
