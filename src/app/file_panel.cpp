#include "app/file_panel.h"
#include "app/editor_state.h"
#include "app/file_dialog.h"
#include "core/project_file.h"
#include "core/image_importer.h"
#include "core/exporter.h"

#include <imgui.h>
#include <cstring>
#include <string>

// ── Main draw ─────────────────────────────────────────────────────────────────

void FilePanel::draw(EditorState& state, const LayoutRect& rect) {
    ImGui::SetNextWindowPos(rect.pos(), ImGuiCond_Always);
    ImGui::SetNextWindowSize(rect.size(), ImGuiCond_Always);

    ImGui::Begin("File & Import", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse);

    drawProjectSection(state);
    drawImportSection(state);
    drawExportSection(state);

    // ── Help ──────────────────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Shortcuts:");
    ImGui::BulletText("[B] Brush  [F] Fill  [I] Eyedrop");
    ImGui::BulletText("[Ctrl+Z] Undo  [Ctrl+Y] Redo");
    ImGui::BulletText("[Ctrl+S] Save  [Ctrl+O] Load");
    ImGui::BulletText("Scroll: Zoom | R/M-drag: Pan");

    ImGui::End();
}

// ── Project ───────────────────────────────────────────────────────────────────

void FilePanel::drawProjectSection(EditorState& state) {
    ImGui::Text("Project");
    ImGui::InputText("##projPath", state.projectPath, sizeof(state.projectPath));
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

void FilePanel::drawImportSection(EditorState& state) {
    ImGui::Separator();
    ImGui::Text("Import Image");
    ImGui::InputText("##importPath", state.importPath, sizeof(state.importPath));
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
    ImGui::SliderInt("Width##import", &state.importWidth, 1, 256);
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
    ImGui::SliderInt("BG Tol##bg", &state.importBgTolerance, 0, 200);
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
        state.importStatus = "Removed " + std::to_string(cleared) + " bg beads";
        state.textureCache.markDirty();
    }
    ImGui::SameLine();
    ImGui::SliderInt("Blob##blob", &state.importBlobSize, 1, 50);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Max isolated blob size to remove");
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

    if (!state.importStatus.empty()) {
        ImGui::TextWrapped("%s", state.importStatus.c_str());
    }
}

// ── Export ─────────────────────────────────────────────────────────────────────

void FilePanel::drawExportSection(EditorState& state) {
    ImGui::Separator();
    ImGui::Text("Export");
    ImGui::InputText("##exportPath", state.exportPath, sizeof(state.exportPath));
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
    ImGui::SliderInt("Bead px##exp", &state.exportBeadPx, 4, 64);
    ImGui::Combo("Style##exp", &state.exportStyle, "Flat (grid)\0Bead (3D)\0");

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
