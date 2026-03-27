#include "app/control_panel.h"
#include "app/editor_state.h"
#include "app/file_dialog.h"
#include "core/project_file.h"
#include "core/image_importer.h"
#include "core/exporter.h"

#include <imgui.h>
#include <cmath>
#include <cstdio>
#include <string>
#include <cstring>
#include <algorithm>
#include <vector>

// ── Main draw ─────────────────────────────────────────────────────────────────

void ControlPanel::draw(EditorState& state) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("Controls");

    ImGui::Text("FPS: %.1f (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);

    drawProjectSection(state);
    drawImportSection(state);
    drawExportSection(state);
    drawGridSizeSection(state);
    drawPegboardSection(state);
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
    ImGui::SameLine();
    if (ImGui::Button("...##project")) {
        char const* filter[1] = {"*.pin"};
        std::string path = FileDialog::openFile(
            "Select Project File", state.projectPath, 1, filter, "Pin files");
        if (!path.empty()) {
            std::strncpy(state.projectPath, path.c_str(), sizeof(state.projectPath) - 1);
            state.projectPath[sizeof(state.projectPath) - 1] = '\0';
        }
    }

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
            state.textureCache.markDirty();
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
    ImGui::SameLine();
    if (ImGui::Button("...##import")) {
        char const* filters[8] = {
            "*.png","*.jpg","*.jpeg","*.bmp","*.tga","*.gif","*.psd","*.hdr"};
        std::string path = FileDialog::openFile(
            "Select Image", state.importPath, 8, filters, "Image files");
        if (!path.empty()) {
            std::strncpy(state.importPath, path.c_str(), sizeof(state.importPath) - 1);
            state.importPath[sizeof(state.importPath) - 1] = '\0';
        }
    }
    ImGui::SliderInt("Width (beads)", &state.importWidth, 1, 256);
    ImGui::Combo("Sampling", &state.importSampling,
                 "Point Sample\0Area Average\0");
    ImGui::Combo("Color Match", &state.importColorMatch,
                 "Euclidean RGB\0Redmean\0");

    if (ImGui::Button("Import")) {
        std::string filePath(state.importPath);
        if (!filePath.empty()) {
            SamplingMethod method = (state.importSampling == 0)
                ? SamplingMethod::PointSample
                : SamplingMethod::AreaAverage;
            ColorMatchMethod cmMethod = (state.importColorMatch == 0)
                ? ColorMatchMethod::EuclideanRGB
                : ColorMatchMethod::Redmean;
            state.undoManager.saveSnapshot(state.beadGrid.cols(),
                                            state.beadGrid.rows(),
                                            state.beadGrid.cells());
            auto result = ImageImporter::import(filePath, state.importWidth,
                                                 state.palette, state.beadGrid,
                                                 method, cmMethod);
            state.importStatus = result.message;
            if (result.success) {
                state.syncGridDims();
                state.viewCentred = false;
                state.undoManager.clear();
                state.textureCache.markDirty();
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

    // ── Background removal ────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::SliderInt("BG Tolerance", &state.importBgTolerance, 0, 200);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Colour distance threshold (0 = exact match only, "
                          "higher = more aggressive)");
    }
    if (ImGui::Button("Remove BG")) {
        state.undoManager.saveSnapshot(state.beadGrid.cols(),
                                        state.beadGrid.rows(),
                                        state.beadGrid.cells());
        int cleared = state.beadGrid.removeBackground(state.palette,
                                                       state.importBgTolerance);
        state.undoManager.commitEdit(state.beadGrid.cols(),
                                      state.beadGrid.rows(),
                                      state.beadGrid.cells());
        state.importStatus = "Removed " + std::to_string(cleared) + " background beads";
        state.textureCache.markDirty();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Flood-fill from all edges to remove background.\n"
                          "Cells whose colour is within 'BG Tolerance' of\n"
                          "adjacent background cells are cleared to Empty.");
    }

    // ── Remove isolated small blobs ───────────────────────────────────────────
    ImGui::SliderInt("Max Blob Size", &state.importBlobSize, 1, 50);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Connected regions with this many cells or fewer\n"
                          "will be removed. Useful for cleaning up stray beads\n"
                          "left after background removal.");
    }
    if (ImGui::Button("Remove Blobs")) {
        state.undoManager.saveSnapshot(state.beadGrid.cols(),
                                        state.beadGrid.rows(),
                                        state.beadGrid.cells());
        int cleared = state.beadGrid.removeSmallBlobs(state.importBlobSize);
        state.undoManager.commitEdit(state.beadGrid.cols(),
                                      state.beadGrid.rows(),
                                      state.beadGrid.cells());
        state.importStatus = "Removed " + std::to_string(cleared) + " isolated beads";
        state.textureCache.markDirty();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Delete small isolated colour blobs.\n"
                          "Any connected region of non-empty cells\n"
                          "with area <= 'Max Blob Size' is cleared.");
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
    ImGui::SameLine();
    if (ImGui::Button("...##export")) {
        char const* filter[1] = {"*.png"};
        std::string path = FileDialog::saveFile(
            "Export As", state.exportPath, 1, filter, "PNG files");
        if (!path.empty()) {
            std::strncpy(state.exportPath, path.c_str(), sizeof(state.exportPath) - 1);
            state.exportPath[sizeof(state.exportPath) - 1] = '\0';
        }
    }
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
    sizeChanged |= ImGui::SliderInt("Columns", &state.gridCols, 1, 256);
    sizeChanged |= ImGui::SliderInt("Rows",    &state.gridRows, 1, 256);
    if (sizeChanged) {
        state.beadGrid.resize(state.gridCols, state.gridRows);
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
}

// ── Pegboard ──────────────────────────────────────────────────────────────────

void ControlPanel::drawPegboardSection(EditorState& state) {
    ImGui::Separator();
    ImGui::Text("Pegboard");

    // Board size selector
    static const int sizes[]    = { 29, 57 };
    static const char* labels[] = { "29x29 (Small)", "57x57 (Large)" };
    if (ImGui::Combo("Board Size", &state.pegboardSizeIdx, labels, 2)) {
        state.pegboardSize  = sizes[state.pegboardSizeIdx];
        state.pegboardDirty = true;
    }

    // Recalculate layout when grid or board size changes
    if (state.pegboardDirty) {
        state.pegboardManager.update(state.beadGrid.cols(),
                                      state.beadGrid.rows(),
                                      state.pegboardSize);
        // Clamp current board index
        int total = state.pegboardManager.totalTiles();
        if (state.currentBoard >= total) state.currentBoard = total - 1;
        if (state.currentBoard < 0)     state.currentBoard = 0;
        state.pegboardDirty = false;
    }

    int total = state.pegboardManager.totalTiles();
    ImGui::Text("Layout: %d x %d  (%d boards)",
                state.pegboardManager.tileCountX(),
                state.pegboardManager.tileCountY(),
                total);

    ImGui::Checkbox("Show Board Lines", &state.showBoardOverlay);

    if (total <= 0) return;

    // ── Board navigation ──────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Text("Navigate");

    // Prev / Next buttons
    if (state.currentBoard <= 0) ImGui::BeginDisabled();
    if (ImGui::Button("<< Prev")) {
        state.currentBoard--;
    }
    if (state.currentBoard <= 0) ImGui::EndDisabled();

    ImGui::SameLine();

    if (state.currentBoard >= total - 1) ImGui::BeginDisabled();
    if (ImGui::Button("Next >>")) {
        state.currentBoard++;
    }
    if (state.currentBoard >= total - 1) ImGui::EndDisabled();

    // Board selector combo
    const auto& currentTile = state.pegboardManager.tile(state.currentBoard);
    std::string currentLabel = PegboardManager::tileLabel(currentTile);
    if (ImGui::BeginCombo("Board", currentLabel.c_str())) {
        for (int i = 0; i < total; ++i) {
            const auto& t = state.pegboardManager.tile(i);
            std::string label = PegboardManager::tileLabel(t);
            int beads = PegboardManager::countBeads(t, state.beadGrid);
            label += " (" + std::to_string(beads) + ")";
            bool selected = (i == state.currentBoard);
            if (ImGui::Selectable(label.c_str(), selected)) {
                state.currentBoard = i;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // ── Tile grid mini-map ────────────────────────────────────────────────────
    {
        int tx = state.pegboardManager.tileCountX();
        int ty = state.pegboardManager.tileCountY();
        float cellSize = 24.0f;
        float spacing  = 2.0f;
        float totalW   = tx * (cellSize + spacing) - spacing;

        // Centre the mini-map
        float avail = ImGui::GetContentRegionAvail().x;
        float startX = (avail - totalW) * 0.5f;
        if (startX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        for (int r = 0; r < ty; ++r) {
            for (int c = 0; c < tx; ++c) {
                int idx = r * tx + c;
                float x0 = origin.x + c * (cellSize + spacing);
                float y0 = origin.y + r * (cellSize + spacing);
                float x1 = x0 + cellSize;
                float y1 = y0 + cellSize;

                bool isCurrent = (idx == state.currentBoard);

                // Background
                const auto& t = state.pegboardManager.tile(idx);
                int beads = PegboardManager::countBeads(t, state.beadGrid);
                ImU32 bgCol;
                if (beads == 0) {
                    bgCol = IM_COL32(60, 60, 60, 255);      // empty
                } else {
                    // Shade by fill ratio
                    int area = t.tileCols() * t.tileRows();
                    float ratio = static_cast<float>(beads) / std::max(1, area);
                    int g = 80 + static_cast<int>(ratio * 120);
                    bgCol = IM_COL32(40, g, 90, 255);
                }

                dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), bgCol);

                // Highlight current board
                if (isCurrent) {
                    dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1),
                                IM_COL32(255, 220, 50, 255), 0.0f, 0, 2.5f);
                } else {
                    dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1),
                                IM_COL32(120, 120, 120, 255));
                }

                // Row-column label inside the cell
                char label[16];
                char rowC = static_cast<char>('A' + (r % 26));
                std::snprintf(label, sizeof(label), "%c%d", rowC, c + 1);
                ImVec2 textSize = ImGui::CalcTextSize(label);
                dl->AddText(ImVec2(x0 + (cellSize - textSize.x) * 0.5f,
                                   y0 + (cellSize - textSize.y) * 0.5f),
                            isCurrent ? IM_COL32(255, 255, 200, 255)
                                      : IM_COL32(200, 200, 200, 255),
                            label);
            }
        }

        // Reserve vertical space for the mini-map
        float mapH = ty * (cellSize + spacing) - spacing;
        ImGui::Dummy(ImVec2(totalW, mapH));

        // Click on the mini-map to navigate
        ImVec2 mousePos = ImGui::GetMousePos();
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            for (int r = 0; r < ty; ++r) {
                for (int c = 0; c < tx; ++c) {
                    float x0 = origin.x + c * (cellSize + spacing);
                    float y0 = origin.y + r * (cellSize + spacing);
                    if (mousePos.x >= x0 && mousePos.x < x0 + cellSize &&
                        mousePos.y >= y0 && mousePos.y < y0 + cellSize) {
                        state.currentBoard = r * tx + c;
                    }
                }
            }
        }
    }

    // ── Current board statistics ──────────────────────────────────────────────
    ImGui::Spacing();
    {
        const auto& t = state.pegboardManager.tile(state.currentBoard);
        std::string lbl = PegboardManager::tileLabel(t);
        ImGui::Text("%s  (%dx%d)", lbl.c_str(), t.tileCols(), t.tileRows());
        ImGui::Text("Cells: [%d,%d) x [%d,%d)",
                    t.startCol, t.endCol, t.startRow, t.endRow);

        int beads = PegboardManager::countBeads(t, state.beadGrid);
        int area  = t.tileCols() * t.tileRows();
        ImGui::Text("Beads: %d / %d  (%.0f%%)", beads, area,
                    area > 0 ? 100.0f * beads / area : 0.0f);

        // Per-colour breakdown (scrollable)
        auto counts = PegboardManager::countBeadsPerColour(
            t, state.beadGrid, state.palette.size());
        struct CS { int idx; int count; };
        std::vector<CS> used;
        for (int i = 1; i < static_cast<int>(counts.size()); ++i) {
            if (counts[i] > 0) used.push_back({ i, counts[i] });
        }
        std::sort(used.begin(), used.end(),
                  [](const CS& a, const CS& b) { return a.count > b.count; });

        if (!used.empty()) {
            ImGui::Text("Colours: %d", static_cast<int>(used.size()));
            if (ImGui::BeginChild("##boardColours", ImVec2(0, 100),
                                  ImGuiChildFlags_Border)) {
                for (const auto& cs : used) {
                    ImU32 col = state.palette.color(cs.idx);
                    ImVec4 colVec = ImGui::ColorConvertU32ToFloat4(col);
                    ImGui::PushID(cs.idx);
                    ImGui::ColorButton("##c", colVec,
                                       ImGuiColorEditFlags_NoTooltip |
                                       ImGuiColorEditFlags_NoBorder,
                                       ImVec2(12, 12));
                    ImGui::PopID();
                    ImGui::SameLine();
                    ImGui::Text("%s x%d", state.palette.name(cs.idx), cs.count);
                }
            }
            ImGui::EndChild();
        }
    }

    // ── Navigate to board on canvas ───────────────────────────────────────────
    if (ImGui::Button("Focus Board")) {
        const auto& t = state.pegboardManager.tile(state.currentBoard);
        float cx = (t.startCol + t.endCol) * 0.5f;
        float cy = (t.startRow + t.endRow) * 0.5f;
        float bw = static_cast<float>(t.tileCols());
        float bh = static_cast<float>(t.tileRows());
        // Centre the canvas view on this tile
        ImVec2 canvasSize = state.canvas.size();
        float scaleX = canvasSize.x / bw;
        float scaleY = canvasSize.y / bh;
        // We don't directly set zoom here — just centre the view
        state.canvas.centreView(static_cast<float>(state.beadGrid.cols()),
                                static_cast<float>(state.beadGrid.rows()));
        // Override with tile-specific centre — use a small helper approach:
        // We'll re-centre after the general centreView.
        // For now just mark viewCentred false so next frame re-centres.
        state.viewCentred = false;
        // Store tile centre for focused centering
        state.focusBoardRequested = true;
        state.focusBoardCentreX   = cx;
        state.focusBoardCentreY   = cy;
        state.focusBoardWidth     = bw;
        state.focusBoardHeight    = bh;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Centre the canvas view on the selected board");
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
}

// ── Palette ───────────────────────────────────────────────────────────────────

void ControlPanel::drawPaletteSection(EditorState& state) {
    ImGui::Separator();
    ImGui::Text("Palette");

    // ── Brand / file selector ─────────────────────────────────────────────────
    if (!state.paletteFiles.empty()) {
        // Build a combo label from the current selection
        const char* preview = state.paletteFiles[state.paletteFileIndex].label.c_str();
        if (ImGui::BeginCombo("Brand", preview)) {
            for (int i = 0; i < static_cast<int>(state.paletteFiles.size()); ++i) {
                bool selected = (i == state.paletteFileIndex);
                if (ImGui::Selectable(state.paletteFiles[i].label.c_str(), selected)) {
                    if (i != state.paletteFileIndex) {
                        state.paletteFileIndex = i;
                        state.palette.loadFromFile(state.paletteFiles[i].path);
                        // Clamp selected colour to the new palette size
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
