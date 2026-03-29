#include "app/canvas_panel.h"
#include "app/editor_state.h"
#include "app/file_dialog.h"
#include "core/project_file.h"

#include <imgui.h>
#include <cmath>
#include <cstdlib> // std::abs
#include <cstring>
#include <string>

// ── Main draw ─────────────────────────────────────────────────────────────────

void CanvasPanel::draw(EditorState& state, const LayoutRect& rect) {
    ImGui::SetNextWindowPos(rect.pos(), ImGuiCond_Always);
    ImGui::SetNextWindowSize(rect.size(), ImGuiCond_Always);

    ImGui::Begin("Canvas", nullptr,
                 ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoScrollWithMouse |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse);

    if (state.canvas.begin()) {
        // Centre the view on the first valid frame
        if (!state.viewCentred) {
            state.canvas.centreView(static_cast<float>(state.beadGrid.cols()),
                                    static_cast<float>(state.beadGrid.rows()));
            state.viewCentred = true;
        }

        // Handle "Focus Board" request from control panel
        if (state.focusBoardRequested) {
            state.canvas.focusRect(state.focusBoardCentreX,
                                   state.focusBoardCentreY,
                                   state.focusBoardWidth,
                                   state.focusBoardHeight);
            state.focusBoardRequested = false;
            state.viewCentred = true;
        }

        handleShortcuts(state);
        handleMouseInteraction(state);

        // Update the cached texture (only rebuilds when dirty)
        state.textureCache.update(state.beadGrid, state.palette);

        // Draw the cached texture via ImGui, mapped to world coordinates
        drawCachedTexture(state);

        // Draw reference image overlay (semi-transparent trace layer)
        if (state.showReferenceOverlay && state.referenceOverlay.isLoaded()) {
            drawReferenceOverlay(state);
        }

        // Draw pegboard overlay lines
        if (state.showBoardOverlay) {
            drawPegboardOverlay(state);
        }

        // Draw progress overlay (checkmarks on completed beads)
        if (state.showProgressOverlay) {
            drawProgressOverlay(state);
        }

        state.canvas.end();
    }

    ImGui::End();
}

// ── Keyboard shortcuts ────────────────────────────────────────────────────────

void CanvasPanel::handleShortcuts(EditorState& state) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) return;

    // Tool switching
    if (ImGui::IsKeyPressed(ImGuiKey_B)) state.currentTool = Tool::Brush;
    if (ImGui::IsKeyPressed(ImGuiKey_F)) state.currentTool = Tool::FloodFill;
    if (ImGui::IsKeyPressed(ImGuiKey_I)) state.currentTool = Tool::Eyedropper;
    if (ImGui::IsKeyPressed(ImGuiKey_M)) state.currentTool = Tool::MarkDone;

    // Toggle progress overlay
    if (ImGui::IsKeyPressed(ImGuiKey_P)) {
        state.showProgressOverlay = !state.showProgressOverlay;
    }

    // Toggle reference (trace) overlay
    if (ImGui::IsKeyPressed(ImGuiKey_T)) {
        if (state.referenceOverlay.isLoaded()) {
            state.showReferenceOverlay = !state.showReferenceOverlay;
        }
    }

    // Save: Ctrl+S
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        char const* filter[1] = {"*.pin"};
        std::string path = FileDialog::saveFile(
            "Save Project", state.projectPath, 1, filter, "Pin files");
        if (!path.empty()) {
            std::strncpy(state.projectPath, path.c_str(), sizeof(state.projectPath) - 1);
            state.projectPath[sizeof(state.projectPath) - 1] = '\0';
            auto result = ProjectFile::save(state.projectPath, state.beadGrid,
                                             state.palette,
                                             state.progressTracker);
            state.projectStatus = result.message;
        }
    }

    // Load: Ctrl+O
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O)) {
        char const* filter[1] = {"*.pin"};
        std::string path = FileDialog::openFile(
            "Open Project", state.projectPath, 1, filter, "Pin files");
        if (!path.empty()) {
            std::strncpy(state.projectPath, path.c_str(), sizeof(state.projectPath) - 1);
            state.projectPath[sizeof(state.projectPath) - 1] = '\0';
            auto result = ProjectFile::load(state.projectPath, state.beadGrid,
                                             state.palette,
                                             state.progressTracker);
            state.projectStatus = result.message;
            if (result.success) {
                state.syncGridDims();
                if (state.selectedColor >= state.palette.size())
                    state.selectedColor = 1;
                state.undoManager.clear();
                state.viewCentred = false;
                state.textureCache.markDirty();
            }
        }
    }

    // Undo: Ctrl+Z (without Shift)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) &&
        !io.KeyShift && state.undoManager.canUndo()) {
        auto snapshot = state.undoManager.undo(state.beadGrid.cols(),
                                               state.beadGrid.rows(),
                                               state.beadGrid.cells());
        state.beadGrid.restoreFrom(snapshot.cols, snapshot.rows, snapshot.cells);
        state.syncGridDims();
        state.textureCache.markDirty();
    }

    // Redo: Ctrl+Y or Ctrl+Shift+Z
    if (io.KeyCtrl &&
        (ImGui::IsKeyPressed(ImGuiKey_Y) ||
         (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z))) &&
        state.undoManager.canRedo()) {
        auto snapshot = state.undoManager.redo(state.beadGrid.cols(),
                                               state.beadGrid.rows(),
                                               state.beadGrid.cells());
        state.beadGrid.restoreFrom(snapshot.cols, snapshot.rows, snapshot.cells);
        state.syncGridDims();
        state.textureCache.markDirty();
    }
}

// ── Mouse interaction ─────────────────────────────────────────────────────────

void CanvasPanel::handleMouseInteraction(EditorState& state) {
    if (state.canvas.isClicked() || state.canvas.isDragging()) {
        ImVec2 mw = state.canvas.mouseWorldPos();
        int col = static_cast<int>(std::floor(mw.x));
        int row = static_cast<int>(std::floor(mw.y));

        if (state.beadGrid.inBounds(col, row)) {
            switch (state.currentTool) {
            case Tool::Brush:
                // Save snapshot at the start of a brush stroke
                if (state.canvas.isClicked() && !state.brushStrokeActive) {
                    state.undoManager.saveSnapshot(state.beadGrid.cols(),
                                                    state.beadGrid.rows(),
                                                    state.beadGrid.cells());
                    state.brushStrokeActive = true;
                    state.lastBrushCol = -1;
                    state.lastBrushRow = -1;
                }

                // Bresenham interpolation between last and current position
                if (state.brushStrokeActive &&
                    state.lastBrushCol >= 0 && state.lastBrushRow >= 0 &&
                    (col != state.lastBrushCol || row != state.lastBrushRow)) {
                    // Walk from (lastCol, lastRow) to (col, row)
                    int x0 = state.lastBrushCol, y0 = state.lastBrushRow;
                    int x1 = col,                y1 = row;
                    int dx = std::abs(x1 - x0);
                    int dy = -std::abs(y1 - y0);
                    int sx = (x0 < x1) ? 1 : -1;
                    int sy = (y0 < y1) ? 1 : -1;
                    int err = dx + dy;

                    // Skip the starting point (already painted last frame)
                    while (x0 != x1 || y0 != y1) {
                        int e2 = 2 * err;
                        if (e2 >= dy) { err += dy; x0 += sx; }
                        if (e2 <= dx) { err += dx; y0 += sy; }
                        state.beadGrid.paintBrush(x0, y0,
                                                   static_cast<uint8_t>(state.selectedColor),
                                                   state.brushSize);
                    }
                } else {
                    state.beadGrid.paintBrush(col, row,
                                               static_cast<uint8_t>(state.selectedColor),
                                               state.brushSize);
                }

                state.lastBrushCol = col;
                state.lastBrushRow = row;
                state.textureCache.markDirty();
                break;

            case Tool::FloodFill:
                // Only fill on click, not drag
                if (state.canvas.isClicked()) {
                    state.undoManager.saveSnapshot(state.beadGrid.cols(),
                                                    state.beadGrid.rows(),
                                                    state.beadGrid.cells());
                    state.beadGrid.floodFill(col, row,
                                              static_cast<uint8_t>(state.selectedColor));
                    state.undoManager.discardIfUnchanged(state.beadGrid.cells());
                    state.textureCache.markDirty();
                }
                break;

            case Tool::Eyedropper:
                if (state.canvas.isClicked()) {
                    state.selectedColor = state.beadGrid.get(col, row);
                    state.currentTool = Tool::Brush;
                }
                break;

            case Tool::MarkDone:
                // Mark/unmark beads as completed using the brush size
                state.progressTracker.markBrush(col, row,
                                                 state.markDoneValue,
                                                 state.brushSize);
                break;
            }
        }
    }

    // End brush stroke when mouse is released
    if (state.brushStrokeActive && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        state.brushStrokeActive = false;
        state.lastBrushCol = -1;
        state.lastBrushRow = -1;
        state.undoManager.discardIfUnchanged(state.beadGrid.cells());
    }
}

// ── Draw cached texture mapped to world coordinates ───────────────────────────

void CanvasPanel::drawCachedTexture(EditorState& state) {
    GLuint texId = state.textureCache.textureId();
    if (texId == 0) return;

    // The texture covers world rect (0,0) to (cols, rows)
    float cols = static_cast<float>(state.textureCache.gridCols());
    float rows = static_cast<float>(state.textureCache.gridRows());

    // Convert world corners to screen coordinates
    ImVec2 topLeft  = state.canvas.worldToScreen(0.0f, 0.0f);
    ImVec2 botRight = state.canvas.worldToScreen(cols, rows);

    // Draw the texture as a quad via the ImGui DrawList
    ImDrawList* dl = state.canvas.drawList();
    if (dl) {
        dl->AddImage(
            static_cast<ImTextureID>(texId),
            topLeft, botRight,
            ImVec2(0.0f, 0.0f),   // UV top-left
            ImVec2(1.0f, 1.0f));  // UV bottom-right
    }
}

// ── Draw pegboard overlay lines on the canvas ─────────────────────────────────

void CanvasPanel::drawPegboardOverlay(EditorState& state) {
    ImDrawList* dl = state.canvas.drawList();
    if (!dl) return;

    const auto& mgr = state.pegboardManager;
    if (mgr.totalTiles() <= 1) return;  // single board — nothing to draw

    int pegSize = mgr.pegboardSize();
    int gridCols = state.beadGrid.cols();
    int gridRows = state.beadGrid.rows();
    int tilesX = mgr.tileCountX();
    int tilesY = mgr.tileCountY();

    ImU32 lineCol     = IM_COL32(255, 200, 50, 180);
    ImU32 currentCol  = IM_COL32(255, 100, 50, 220);
    float lineThick   = 2.0f;
    float currentThick = 3.5f;

    // Draw vertical board-boundary lines
    for (int tx = 1; tx < tilesX; ++tx) {
        float wx = static_cast<float>(tx * pegSize);
        ImVec2 top = state.canvas.worldToScreen(wx, 0.0f);
        ImVec2 bot = state.canvas.worldToScreen(wx, static_cast<float>(gridRows));
        dl->AddLine(top, bot, lineCol, lineThick);
    }

    // Draw horizontal board-boundary lines
    for (int ty = 1; ty < tilesY; ++ty) {
        float wy = static_cast<float>(ty * pegSize);
        ImVec2 left  = state.canvas.worldToScreen(0.0f, wy);
        ImVec2 right = state.canvas.worldToScreen(static_cast<float>(gridCols), wy);
        dl->AddLine(left, right, lineCol, lineThick);
    }

    // Highlight the currently selected board with a thicker rectangle
    if (state.currentBoard >= 0 && state.currentBoard < mgr.totalTiles()) {
        const auto& t = mgr.tile(state.currentBoard);
        ImVec2 tl = state.canvas.worldToScreen(
            static_cast<float>(t.startCol), static_cast<float>(t.startRow));
        ImVec2 br = state.canvas.worldToScreen(
            static_cast<float>(t.endCol), static_cast<float>(t.endRow));
        dl->AddRect(tl, br, currentCol, 0.0f, 0, currentThick);

        // Label in the top-left corner of the board
        std::string label = PegboardManager::tileLabel(t);
        ImVec2 textPos = ImVec2(tl.x + 4.0f, tl.y + 2.0f);
        dl->AddText(textPos, currentCol, label.c_str());
    }
}

// ── Draw progress overlay (checkmarks on completed beads) ─────────────────────

void CanvasPanel::drawProgressOverlay(EditorState& state) {
    ImDrawList* dl = state.canvas.drawList();
    if (!dl) return;

    const auto& tracker = state.progressTracker;
    int cols = state.beadGrid.cols();
    int rows = state.beadGrid.rows();

    // Only draw marks that are visible on screen to save draw calls.
    // Compute the visible world-coordinate bounding box.
    float scale = state.canvas.scale();
    if (scale < 2.0f) return; // too zoomed out to see marks

    // Semi-transparent green overlay + checkmark colour
    ImU32 overlayCol  = IM_COL32(0, 200, 80, 60);
    ImU32 checkCol    = IM_COL32(0, 200, 80, 220);
    float checkThick  = std::max(1.0f, scale * 0.08f);

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            if (!tracker.isDone(col, row)) continue;

            // Skip empty beads — only track non-empty cells
            if (state.beadGrid.get(col, row) == 0) continue;

            float wx = static_cast<float>(col);
            float wy = static_cast<float>(row);
            ImVec2 tl = state.canvas.worldToScreen(wx, wy);
            ImVec2 br = state.canvas.worldToScreen(wx + 1.0f, wy + 1.0f);

            // Quick frustum cull
            if (br.x < 0 || br.y < 0) continue;

            // Draw a semi-transparent green tint over the bead
            dl->AddRectFilled(tl, br, overlayCol);

            // Draw a small checkmark (✓) inside the cell
            float cw = br.x - tl.x;
            float ch = br.y - tl.y;
            float cx = tl.x + cw * 0.5f;
            float cy = tl.y + ch * 0.5f;

            // Checkmark: short stroke from bottom-left to bottom-centre,
            // then long stroke from bottom-centre to top-right.
            ImVec2 pts[3] = {
                ImVec2(cx - cw * 0.22f, cy + ch * 0.02f),  // left arm start
                ImVec2(cx - cw * 0.05f, cy + ch * 0.20f),  // bottom vertex
                ImVec2(cx + cw * 0.25f, cy - ch * 0.22f),  // right arm end
            };

            dl->AddPolyline(pts, 3, checkCol, ImDrawFlags_None, checkThick);
        }
    }
}

// ── Draw reference image overlay (trace layer) ───────────────────────────────

void CanvasPanel::drawReferenceOverlay(EditorState& state) {
    ImDrawList* dl = state.canvas.drawList();
    if (!dl) return;

    GLuint texId = state.referenceOverlay.textureId();
    if (texId == 0) return;

    // The reference image covers the same world rect as the grid: (0,0)→(cols,rows)
    float cols = static_cast<float>(state.beadGrid.cols());
    float rows = static_cast<float>(state.beadGrid.rows());

    ImVec2 topLeft  = state.canvas.worldToScreen(0.0f, 0.0f);
    ImVec2 botRight = state.canvas.worldToScreen(cols, rows);

    // Tint colour: white with user-controlled alpha for opacity
    uint8_t alpha = static_cast<uint8_t>(state.referenceOpacity * 255.0f);
    ImU32 tintCol = IM_COL32(255, 255, 255, alpha);

    dl->AddImage(
        static_cast<ImTextureID>(texId),
        topLeft, botRight,
        ImVec2(0.0f, 0.0f),   // UV top-left
        ImVec2(1.0f, 1.0f),   // UV bottom-right
        tintCol);
}
