#include "app/tools_panel.h"
#include "app/editor_state.h"

#include <imgui.h>
#include <cmath>
#include <cstdio>
#include <algorithm>

// ── Main draw ─────────────────────────────────────────────────────────────────

void ToolsPanel::draw(EditorState& state, const LayoutRect& rect) {
    ImGui::SetNextWindowPos(rect.pos(), ImGuiCond_Always);
    ImGui::SetNextWindowSize(rect.size(), ImGuiCond_Always);

    ImGui::Begin("Tools & Palette", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse);

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("FPS: %.1f (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);

    drawToolsSection(state);
    drawPaletteSection(state);
    drawViewInfoSection(state);

    ImGui::End();
}

// ── Tools ─────────────────────────────────────────────────────────────────────

void ToolsPanel::drawToolsSection(EditorState& state) {
    ImGui::Separator();
    ImGui::Text("Tools");

    // Tool selection buttons (highlighted when active)
    auto toolButton = [&](const char* label, Tool tool) {
        bool active = (state.currentTool == tool);
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
        }
        if (ImGui::Button(label)) {
            state.currentTool = tool;
        }
        if (active) {
            ImGui::PopStyleColor();
        }
    };

    toolButton("Brush [B]", Tool::Brush);
    ImGui::SameLine();
    toolButton("Fill [F]", Tool::FloodFill);
    ImGui::SameLine();
    toolButton("Eyedrop [I]", Tool::Eyedropper);
    ImGui::SameLine();
    toolButton("Mark [M]", Tool::MarkDone);

    // Brush size (only relevant for Brush and MarkDone tools)
    if (state.currentTool == Tool::Brush || state.currentTool == Tool::MarkDone) {
        ImGui::SliderInt("Brush Size", &state.brushSize, 1, 3);
        const char* sizeLabels[] = { "", "1x1", "3x3", "5x5" };
        ImGui::SameLine();
        ImGui::Text("(%s)", sizeLabels[state.brushSize]);
    }

    // Mark mode toggle (only for MarkDone tool)
    if (state.currentTool == Tool::MarkDone) {
        ImGui::Checkbox("Mark as Done", &state.markDoneValue);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Checked = mark beads as done; "
                              "Unchecked = unmark beads");
        }
    }

    // Undo / Redo buttons
    {
        bool canUndo = state.undoManager.canUndo();
        bool canRedo = state.undoManager.canRedo();

        if (!canUndo) ImGui::BeginDisabled();
        if (ImGui::Button("Undo [Ctrl+Z]")) {
            auto snapshot = state.undoManager.undo(state.beadGrid.cols(),
                                                    state.beadGrid.rows(),
                                                    state.beadGrid.cells());
            state.beadGrid.restoreFrom(snapshot.cols, snapshot.rows, snapshot.cells);
            state.syncGridDims();
            state.textureCache.markDirty();
        }
        if (!canUndo) ImGui::EndDisabled();

        ImGui::SameLine();

        if (!canRedo) ImGui::BeginDisabled();
        if (ImGui::Button("Redo [Ctrl+Y]")) {
            auto snapshot = state.undoManager.redo(state.beadGrid.cols(),
                                                    state.beadGrid.rows(),
                                                    state.beadGrid.cells());
            state.beadGrid.restoreFrom(snapshot.cols, snapshot.rows, snapshot.cells);
            state.syncGridDims();
            state.textureCache.markDirty();
        }
        if (!canRedo) ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::TextDisabled("(%zu / %zu)", state.undoManager.undoCount(),
                            state.undoManager.redoCount());
    }

    // ── Grid size ─────────────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::Text("Grid Size");

    bool sizeChanged = false;
    sizeChanged |= ImGui::SliderInt("Columns", &state.gridCols, 1, 256);
    sizeChanged |= ImGui::SliderInt("Rows",    &state.gridRows, 1, 256);
    if (sizeChanged) {
        state.beadGrid.resize(state.gridCols, state.gridRows);
        state.progressTracker.resize(state.gridCols, state.gridRows);
        state.undoManager.clear();
        state.textureCache.markDirty();
        state.pegboardDirty = true;
    }

    if (ImGui::Button("Clear Grid")) {
        state.undoManager.saveSnapshot(state.beadGrid.cols(),
                                        state.beadGrid.rows(),
                                        state.beadGrid.cells());
        state.beadGrid.clear();
        state.undoManager.discardIfUnchanged(state.beadGrid.cells());
        state.textureCache.markDirty();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset View")) {
        state.viewCentred = false;
    }

    // ── Progress tracking ─────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::Text("Progress");
    ImGui::Checkbox("Show Overlay [P]", &state.showProgressOverlay);

    // Count only non-empty beads for progress calculation
    {
        const auto& cells = state.beadGrid.cells();
        const auto& tracker = state.progressTracker;
        int totalBeads = 0;
        int doneBeads  = 0;
        for (int r = 0; r < state.beadGrid.rows(); ++r) {
            for (int c = 0; c < state.beadGrid.cols(); ++c) {
                if (state.beadGrid.get(c, r) != 0) {
                    ++totalBeads;
                    if (tracker.isDone(c, r)) ++doneBeads;
                }
            }
        }
        float pct = (totalBeads > 0)
            ? static_cast<float>(doneBeads) / totalBeads
            : 0.0f;

        char overlay[64];
        std::snprintf(overlay, sizeof(overlay), "%d / %d (%.1f%%)",
                      doneBeads, totalBeads, pct * 100.0f);
        ImGui::ProgressBar(pct, ImVec2(-1, 0), overlay);
    }

    if (ImGui::Button("Clear Marks")) {
        state.progressTracker.clearAll();
    }
    ImGui::SameLine();
    if (ImGui::Button("Mark All")) {
        state.progressTracker.markAll();
    }
}

// ── Palette ───────────────────────────────────────────────────────────────────

void ToolsPanel::drawPaletteSection(EditorState& state) {
    ImGui::Separator();
    ImGui::Text("Palette");

    // ── Brand / file selector ─────────────────────────────────────────────────
    if (!state.paletteFiles.empty()) {
        const char* preview = state.paletteFiles[state.paletteFileIndex].label.c_str();
        if (ImGui::BeginCombo("Brand", preview)) {
            for (int i = 0; i < static_cast<int>(state.paletteFiles.size()); ++i) {
                bool selected = (i == state.paletteFileIndex);
                if (ImGui::Selectable(state.paletteFiles[i].label.c_str(), selected)) {
                    if (i != state.paletteFileIndex) {
                        state.paletteFileIndex = i;
                        state.palette.loadFromFile(state.paletteFiles[i].path);
                        if (state.selectedColor >= state.palette.size()) {
                            state.selectedColor = std::min(1, state.palette.size() - 1);
                        }
                        state.textureCache.markDirty();
                    }
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    ImGui::Text("Selected: %s", state.palette.name(state.selectedColor));

    float buttonSize = 24.0f;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int buttonsPerRow = std::max(1, static_cast<int>(panelWidth / (buttonSize + 4.0f)));

    for (int i = 0; i < state.palette.size(); ++i) {
        ImU32 col = state.palette.color(i);
        ImVec4 colVec = ImGui::ColorConvertU32ToFloat4(col);

        ImGui::PushID(i);

        bool isSelected = (i == state.selectedColor);
        if (isSelected) {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 3.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        }

        if (ImGui::ColorButton(state.palette.name(i), colVec,
                               ImGuiColorEditFlags_NoTooltip,
                               ImVec2(buttonSize, buttonSize))) {
            state.selectedColor = i;
        }

        if (isSelected) {
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s (index %d)", state.palette.name(i), i);
        }

        ImGui::PopID();

        if ((i + 1) % buttonsPerRow != 0 && i + 1 < state.palette.size()) {
            ImGui::SameLine();
        }
    }
}

// ── View info ─────────────────────────────────────────────────────────────────

void ToolsPanel::drawViewInfoSection(EditorState& state) {
    ImGui::Separator();
    ImGui::Text("Zoom: %.1f px/unit", state.canvas.scale());

    if (state.canvas.isHovered()) {
        ImVec2 mw = state.canvas.mouseWorldPos();
        int hoverCol = static_cast<int>(std::floor(mw.x));
        int hoverRow = static_cast<int>(std::floor(mw.y));
        if (state.beadGrid.inBounds(hoverCol, hoverRow)) {
            uint8_t ci = state.beadGrid.get(hoverCol, hoverRow);
            ImGui::Text("Hover: (%d, %d) = %s", hoverCol, hoverRow,
                        state.palette.name(ci));
        }
    }
}
