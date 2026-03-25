#include "core/undo_manager.h"
#include <algorithm>

UndoManager::UndoManager(size_t maxHistory)
    : maxHistory_(maxHistory) {}

void UndoManager::saveSnapshot(int cols, int rows,
                                const std::vector<uint8_t>& cells) {
    // Store the pre-edit state; the actual delta will be computed
    // when commitEdit() (or discardIfUnchanged()) is called.
    pendingCols_  = cols;
    pendingRows_  = rows;
    pendingCells_ = cells;       // deep copy
    hasPending_   = true;
}

void UndoManager::commitEdit(int cols, int rows,
                              const std::vector<uint8_t>& cells) {
    if (!hasPending_) return;
    hasPending_ = false;

    // Build the delta between pre-edit (pending) and post-edit (current)
    GridDelta delta;
    delta.cols    = pendingCols_;
    delta.rows    = pendingRows_;
    delta.newCols = cols;
    delta.newRows = rows;

    const size_t len = std::min(pendingCells_.size(), cells.size());
    for (size_t i = 0; i < len; ++i) {
        if (pendingCells_[i] != cells[i]) {
            delta.deltas.push_back({
                static_cast<int>(i), pendingCells_[i], cells[i]
            });
        }
    }
    // Handle size changes (cells that exist only in old or new grid)
    for (size_t i = len; i < pendingCells_.size(); ++i) {
        delta.deltas.push_back({
            static_cast<int>(i), pendingCells_[i], uint8_t(0)
        });
    }
    for (size_t i = len; i < cells.size(); ++i) {
        delta.deltas.push_back({
            static_cast<int>(i), uint8_t(0), cells[i]
        });
    }

    // Nothing changed — silently discard (no undo entry needed)
    if (delta.deltas.empty() &&
        delta.cols == delta.newCols && delta.rows == delta.newRows) {
        return;
    }

    // Any new edit invalidates the redo history
    redoStack_.clear();

    // Trim oldest entries if we exceed the limit
    if (undoStack_.size() >= maxHistory_) {
        undoStack_.erase(undoStack_.begin());
    }

    undoStack_.push_back(std::move(delta));
    pendingCells_.clear();           // free the temporary copy
    pendingCells_.shrink_to_fit();
}

void UndoManager::discardIfUnchanged(const std::vector<uint8_t>& currentCells) {
    if (!hasPending_) return;
    // commitEdit already silently discards when nothing changed,
    // so we can just delegate.  We use the pending dimensions as the
    // "current" dimensions — they are the same unless a resize happened.
    commitEdit(pendingCols_, pendingRows_, currentCells);
}

bool UndoManager::canUndo() const {
    return !undoStack_.empty();
}

bool UndoManager::canRedo() const {
    return !redoStack_.empty();
}

GridSnapshot UndoManager::undo(int cols, int rows,
                               const std::vector<uint8_t>& currentCells) {
    // Flush any uncommitted pending edit so it is not lost
    if (hasPending_) {
        commitEdit(cols, rows, currentCells);
    }

    const GridDelta& delta = undoStack_.back();

    // Build the reversed delta and push it onto the redo stack
    GridDelta reversed;
    reversed.cols    = delta.newCols;
    reversed.rows    = delta.newRows;
    reversed.newCols = delta.cols;
    reversed.newRows = delta.rows;
    reversed.deltas.reserve(delta.deltas.size());
    for (const auto& d : delta.deltas) {
        reversed.deltas.push_back({ d.index, d.newValue, d.oldValue });
    }
    redoStack_.push_back(std::move(reversed));

    // Apply the undo: restore pre-edit dimensions and patch cells
    GridSnapshot result;
    result.cols  = delta.cols;
    result.rows  = delta.rows;
    result.cells = currentCells;
    result.cells.resize(static_cast<size_t>(delta.cols) * delta.rows, 0);
    for (const auto& d : delta.deltas) {
        if (d.index < static_cast<int>(result.cells.size())) {
            result.cells[d.index] = d.oldValue;
        }
    }

    undoStack_.pop_back();
    return result;
}

GridSnapshot UndoManager::redo(int cols, int rows,
                               const std::vector<uint8_t>& currentCells) {
    const GridDelta& delta = redoStack_.back();

    // Build the reversed delta and push it onto the undo stack
    GridDelta reversed;
    reversed.cols    = delta.newCols;
    reversed.rows    = delta.newRows;
    reversed.newCols = delta.cols;
    reversed.newRows = delta.rows;
    reversed.deltas.reserve(delta.deltas.size());
    for (const auto& d : delta.deltas) {
        reversed.deltas.push_back({ d.index, d.newValue, d.oldValue });
    }
    undoStack_.push_back(std::move(reversed));

    // Apply the redo: restore post-edit dimensions and patch cells
    GridSnapshot result;
    result.cols  = delta.cols;
    result.rows  = delta.rows;
    result.cells = currentCells;
    result.cells.resize(static_cast<size_t>(delta.cols) * delta.rows, 0);
    for (const auto& d : delta.deltas) {
        if (d.index < static_cast<int>(result.cells.size())) {
            result.cells[d.index] = d.oldValue;
        }
    }

    redoStack_.pop_back();
    return result;
}

void UndoManager::clear() {
    undoStack_.clear();
    redoStack_.clear();
    hasPending_ = false;
    pendingCells_.clear();
    pendingCells_.shrink_to_fit();
}
