#include "core/undo_manager.h"
#include <algorithm>

UndoManager::UndoManager(size_t maxHistory)
    : maxHistory_(maxHistory) {}

void UndoManager::saveSnapshot(int cols, int rows,
                                const std::vector<uint8_t>& cells) {
    // Any new edit invalidates the redo history
    redoStack_.clear();

    // Trim oldest entries if we exceed the limit
    if (undoStack_.size() >= maxHistory_) {
        undoStack_.erase(undoStack_.begin());
    }

    undoStack_.push_back({ cols, rows, cells });
}

void UndoManager::discardIfUnchanged(const std::vector<uint8_t>& currentCells) {
    if (undoStack_.empty()) return;
    if (undoStack_.back().cells == currentCells) {
        undoStack_.pop_back();
    }
}

bool UndoManager::canUndo() const {
    return !undoStack_.empty();
}

bool UndoManager::canRedo() const {
    return !redoStack_.empty();
}

GridSnapshot UndoManager::undo(int cols, int rows,
                               const std::vector<uint8_t>& currentCells) {
    // Push current state onto redo stack before restoring
    redoStack_.push_back({ cols, rows, currentCells });

    GridSnapshot snapshot = std::move(undoStack_.back());
    undoStack_.pop_back();
    return snapshot;
}

GridSnapshot UndoManager::redo(int cols, int rows,
                               const std::vector<uint8_t>& currentCells) {
    // Push current state onto undo stack before restoring
    undoStack_.push_back({ cols, rows, currentCells });

    GridSnapshot snapshot = std::move(redoStack_.back());
    redoStack_.pop_back();
    return snapshot;
}

void UndoManager::clear() {
    undoStack_.clear();
    redoStack_.clear();
}
