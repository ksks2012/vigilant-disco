#include "app/canvas_panel.h"
#include "app/editor_state.h"
#include "app/window.h"
#include "core/project_file.h"

#include <imgui.h>
#include <cmath>

// ── Main draw ─────────────────────────────────────────────────────────────────

void CanvasPanel::draw(EditorState& state, const Window& window) {
    ImGui::SetNextWindowPos(ImVec2(280, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(
        ImVec2(static_cast<float>(window.getWidth()) - 280.0f,
               static_cast<float>(window.getHeight())),
        ImGuiCond_FirstUseEver);

    ImGui::Begin("Canvas", nullptr,
                 ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoScrollWithMouse);

    if (state.canvas.begin()) {
        // Centre the view on the first valid frame
        if (!state.viewCentred) {
            state.canvas.centreView(static_cast<float>(state.beadGrid.cols()),
                                    static_cast<float>(state.beadGrid.rows()));
            state.viewCentred = true;
        }

        handleShortcuts(state);
        handleMouseInteraction(state);

        // Update the cached texture (only rebuilds when dirty)
        state.textureCache.update(state.beadGrid, state.palette);

        // Draw the cached texture via ImGui, mapped to world coordinates
        drawCachedTexture(state);

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

    // Save: Ctrl+S
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        auto result = ProjectFile::save(state.projectPath, state.beadGrid,
                                         state.palette);
        state.projectStatus = result.message;
    }

    // Load: Ctrl+O
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O)) {
        auto result = ProjectFile::load(state.projectPath, state.beadGrid,
                                         state.palette);
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
                }
                state.beadGrid.paintBrush(col, row,
                                           static_cast<uint8_t>(state.selectedColor),
                                           state.brushSize);
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
            }
        }
    }

    // End brush stroke when mouse is released
    if (state.brushStrokeActive && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        state.brushStrokeActive = false;
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
