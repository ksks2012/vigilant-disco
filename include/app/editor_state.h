#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include "core/bead_grid.h"
#include "core/palette.h"
#include "core/undo_manager.h"
#include "rendering/grid_renderer.h"
#include "rendering/bead_texture_cache.h"
#include "app/canvas.h"

#include <string>

// Shared editor state accessible by all UI panels.
// Owned by main(), passed by reference to panel draw calls.
struct EditorState {
    // ── Core data ─────────────────────────────────────────────────────────────
    Canvas           canvas;
    BeadGrid         beadGrid;
    GridRenderer     gridRenderer;
    BeadTextureCache textureCache;
    Palette          palette;
    UndoManager      undoManager{100};

    // ── Grid dimensions (kept in sync with beadGrid) ──────────────────────────
    int gridCols = 29;
    int gridRows = 29;

    // ── Tool state ────────────────────────────────────────────────────────────
    Tool currentTool    = Tool::Brush;
    int  brushSize      = 1;     // 1 = 1x1, 2 = 3x3, 3 = 5x5
    int  selectedColor  = 1;     // palette index for painting

    // ── View ──────────────────────────────────────────────────────────────────
    bool viewCentred = false;

    // ── Brush stroke tracking (for single-undo-per-drag) ──────────────────────
    bool brushStrokeActive = false;
    int  lastBrushCol = -1;   // previous frame brush position for interpolation
    int  lastBrushRow = -1;

    // ── Project file ──────────────────────────────────────────────────────────
    char        projectPath[512] = "project.pin";
    std::string projectStatus;

    // ── Image import ──────────────────────────────────────────────────────────
    char        importPath[512] = "";
    int         importWidth     = 29;
    std::string importStatus;

    // ── Export ─────────────────────────────────────────────────────────────────
    char        exportPath[512] = "export.png";
    int         exportBeadPx    = 20;
    int         exportStyle     = 1;   // 0 = Flat, 1 = Bead
    std::string exportStatus;

    // ── Helpers ───────────────────────────────────────────────────────────────
    // Sync gridCols/gridRows after a load or import that changes grid size
    void syncGridDims() {
        gridCols = beadGrid.cols();
        gridRows = beadGrid.rows();
    }
};

#endif // EDITOR_STATE_H
