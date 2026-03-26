#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include "core/bead_grid.h"
#include "core/palette.h"
#include "core/undo_manager.h"
#include "core/image_importer.h"
#include "rendering/grid_renderer.h"
#include "rendering/bead_texture_cache.h"
#include "app/canvas.h"

#include <string>
#include <vector>

// Describes a palette file that can be selected from the UI.
struct PaletteFileEntry {
    std::string label;     // display name, e.g. "Perler (42 colours)"
    std::string path;      // file path, e.g. "etc/palettes/perler.json"
};

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

    // ── Palette files ─────────────────────────────────────────────────────────
    std::vector<PaletteFileEntry> paletteFiles;
    int  paletteFileIndex = 0;   // currently selected palette file

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
    int         importSampling    = 1;   // 0 = PointSample, 1 = AreaAverage
    int         importColorMatch  = 0;   // 0 = EuclideanRGB, 1 = Redmean
    int         importBgTolerance = 60;  // colour distance threshold for BG removal
    int         importBlobSize    = 5;   // max blob size to remove (in cells)
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
