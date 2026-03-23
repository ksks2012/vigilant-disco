#include "app/control_panel.h"
#include "app/editor_state.h"
#include "core/project_file.h"
#include "core/image_importer.h"
#include "core/exporter.h"

#include <imgui.h>
#include <cmath>
#include <string>
#include <algorithm>

// ── Main draw ─────────────────────────────────────────────────────────────────

void ControlPanel::draw(EditorState& state) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("Controls");

    ImGui::Text("FPS: %.1f (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);

    drawProjectSection(state);
    drawImportSection(state);
    drawExportSection(state);
    drawGridSizeSection(state);
    drawToolsSection(state);
    drawPaletteSection(state);
    drawViewInfoSection(state);
    drawHelpSection();

    ImGui::End();
}

// ── Project ───────────────────────────────────────────────────────────────────

void ControlPanel::drawProjectSection(EditorState& state) {
    ImGui::Separator();
    ImGui::Text("Project");
    ImGui::InputText("Project File", state.projectPath, sizeof(state.projectPath));

    if (ImGui::Button("Save [Ctrl+S]")) {
        auto result = ProjectFile::save(state.projectPath, state.beadGrid,
                                         state.palette);
        state.projectStatus = result.message;
    }
    ImGui::SameLine();
    if (ImGui::Button("Load [Ctrl+O]")) {
        auto result = ProjectFile::load(state.projectPath, state.beadGrid,
                                         state.palette);
        state.projectStatus = result.message;
        if (result.success) {
            state.syncGridDims();
            if (state.selectedColor >= state.palette.size())
                state.selectedColor = 1;
            state.undoManager.clear();
            state.viewCentred = false;
        }
    }

    if (!state.projectStatus.empty()) {
        ImGui::TextWrapped("%s", state.projectStatus.c_str());
    }
}

// ── Import ────────────────────────────────────────────────────────────────────

void ControlPanel::drawImportSection(EditorState& state) {
    ImGui::Separator();
    ImGui::Text("Import Image");
    ImGui::InputText("File Path", state.importPath, sizeof(state.importPath));
    ImGui::SliderInt("Width (beads)", &state.importWidth, 1, 256);

    if (ImGui::Button("Import")) {
        std::string filePath(state.importPath);
        if (!filePath.empty()) {
            state.undoManager.saveSnapshot(state.beadGrid.cols(),
                                            state.beadGrid.rows(),
                                            state.beadGrid.cells());
            auto result = ImageImporter::import(filePath, state.importWidth,
                                                 state.palette, state.beadGrid);
            state.importStatus = result.message;
            if (result.success) {
                state.syncGridDims();
                state.viewCentred = false;
                state.undoManager.clear();
            }
        } else {
            state.importStatus = "Please enter a file path";
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Supports PNG, JPG, BMP, TGA, GIF, PSD, HDR, PIC");
    }

    if (!state.importStatus.empty()) {
        ImGui::TextWrapped("%s", state.importStatus.c_str());
    }
}

// ── Export ─────────────────────────────────────────────────────────────────────

void ControlPanel::drawExportSection(EditorState& state) {
    ImGui::Separator();
    ImGui::Text("Export");
    ImGui::InputText("Export Path", state.exportPath, sizeof(state.exportPath));
    ImGui::SliderInt("Bead Size (px)", &state.exportBeadPx, 4, 64);
    ImGui::Combo("PNG Style", &state.exportStyle, "Flat (grid)\0Bead (3D)\0");

    if (ImGui::Button("Export PNG")) {
        PngStyle style = (state.exportStyle == 0) ? PngStyle::Flat : PngStyle::Bead;
        auto result = Exporter::exportPng(state.exportPath, state.beadGrid,
                                           state.palette, state.exportBeadPx, style);
        state.exportStatus = result.message;
    }
    ImGui::SameLine();
    if (ImGui::Button("Export CSV")) {
        std::string csvPath(state.exportPath);
        auto dotPos = csvPath.rfind('.');
        if (dotPos != std::string::npos) {
            csvPath = csvPath.substr(0, dotPos) + ".csv";
        } else {
            csvPath += ".csv";
        }
        auto result = Exporter::exportCsv(csvPath, state.beadGrid, state.palette);
        state.exportStatus = result.message;
    }

    if (!state.exportStatus.empty()) {
        ImGui::TextWrapped("%s", state.exportStatus.c_str());
    }
}

// ── Grid size ─────────────────────────────────────────────────────────────────

void ControlPanel::drawGridSizeSection(EditorState& state) {
    ImGui::Separator();
    ImGui::Text("Grid Size");

    bool sizeChanged = false;
    sizeChanged |= ImGui::SliderInt("Columns", &state.gridCols, 1, 100);
    sizeChanged |= ImGui::SliderInt("Rows",    &state.gridRows, 1, 100);
    if (sizeChanged) {
        state.beadGrid.resize(state.gridCols, state.gridRows);
        state.undoManager.clear();
    }

    if (ImGui::Button("Clear Grid")) {
        state.undoManager.saveSnapshot(state.beadGrid.cols(),
                                        state.beadGrid.rows(),
                                        state.beadGrid.cells());
        state.beadGrid.clear();
        state.undoManager.discardIfUnchanged(state.beadGrid.cells());
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset View")) {
        state.viewCentred = false;
    }
}

// ── Tools ─────────────────────────────────────────────────────────────────────

void ControlPanel::drawToolsSection(EditorState& state) {
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

    // Brush size (only relevant for Brush tool)
    if (state.currentTool == Tool::Brush) {
        ImGui::SliderInt("Brush Size", &state.brushSize, 1, 3);
        const char* sizeLabels[] = { "", "1x1", "3x3", "5x5" };
        ImGui::SameLine();
        ImGui::Text("(%s)", sizeLabels[state.brushSize]);
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
        }
        if (!canRedo) ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::TextDisabled("(%zu / %zu)", state.undoManager.undoCount(),
                            state.undoManager.redoCount());
    }
}

// ── Palette ───────────────────────────────────────────────────────────────────

void ControlPanel::drawPaletteSection(EditorState& state) {
    ImGui::Separator();
    ImGui::Text("Palette");
    ImGui::Text("Selected: %s", state.palette.name(state.selectedColor));

    float buttonSize = 28.0f;
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

void ControlPanel::drawViewInfoSection(EditorState& state) {
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

// ── Help ──────────────────────────────────────────────────────────────────────

void ControlPanel::drawHelpSection() {
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Controls:");
    ImGui::BulletText("Left-click / drag: Use current tool");
    ImGui::BulletText("Scroll: Zoom in/out");
    ImGui::BulletText("Right / Middle drag: Pan");
    ImGui::BulletText("[B] Brush  [F] Fill  [I] Eyedropper");
    ImGui::BulletText("[Ctrl+Z] Undo  [Ctrl+Y] Redo");
    ImGui::BulletText("[Ctrl+S] Save  [Ctrl+O] Load");
    ImGui::BulletText("Import: Load image into grid");
}
