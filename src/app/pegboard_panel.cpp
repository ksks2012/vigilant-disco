#include "app/pegboard_panel.h"
#include "app/editor_state.h"

#include <imgui.h>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>

// ── Main draw ─────────────────────────────────────────────────────────────────

void PegboardPanel::draw(EditorState& state, const LayoutRect& rect) {
    ImGui::SetNextWindowPos(rect.pos(), ImGuiCond_Always);
    ImGui::SetNextWindowSize(rect.size(), ImGuiCond_Always);

    ImGui::Begin("Pegboard", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse);

    drawSettingsSection(state);
    drawNavigationSection(state);
    drawMiniMap(state);

    ImGui::End();
}

// ── Settings ──────────────────────────────────────────────────────────────────

void PegboardPanel::drawSettingsSection(EditorState& state) {
    // Board size selector
    static const int sizes[]    = { 29, 57 };
    static const char* labels[] = { "29x29 (Small)", "57x57 (Large)" };
    if (ImGui::Combo("Size##peg", &state.pegboardSizeIdx, labels, 2)) {
        state.pegboardSize  = sizes[state.pegboardSizeIdx];
        state.pegboardDirty = true;
    }

    // Recalculate layout when grid or board size changes
    if (state.pegboardDirty) {
        state.pegboardManager.update(state.beadGrid.cols(),
                                      state.beadGrid.rows(),
                                      state.pegboardSize);
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

    ImGui::Checkbox("Show Lines", &state.showBoardOverlay);
}

// ── Navigation ────────────────────────────────────────────────────────────────

void PegboardPanel::drawNavigationSection(EditorState& state) {
    int total = state.pegboardManager.totalTiles();
    if (total <= 0) return;

    ImGui::Separator();

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

    // Focus board button
    if (ImGui::Button("Focus Board")) {
        const auto& t = state.pegboardManager.tile(state.currentBoard);
        float cx = (t.startCol + t.endCol) * 0.5f;
        float cy = (t.startRow + t.endRow) * 0.5f;
        float bw = static_cast<float>(t.tileCols());
        float bh = static_cast<float>(t.tileRows());
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

    // ── Current board info ────────────────────────────────────────────────────
    ImGui::Spacing();
    {
        const auto& t = state.pegboardManager.tile(state.currentBoard);
        std::string lbl = PegboardManager::tileLabel(t);
        ImGui::Text("%s  (%dx%d)", lbl.c_str(), t.tileCols(), t.tileRows());

        int beads = PegboardManager::countBeads(t, state.beadGrid);
        int area  = t.tileCols() * t.tileRows();
        ImGui::Text("Beads: %d / %d  (%.0f%%)", beads, area,
                    area > 0 ? 100.0f * beads / area : 0.0f);

        // Per-board progress
        int done = state.progressTracker.doneCountInRect(
            t.startCol, t.startRow, t.endCol, t.endRow);
        if (beads > 0) {
            float pct = static_cast<float>(done) / beads;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%d / %d (%.0f%%)", done, beads,
                          pct * 100.0f);
            ImGui::Text("Progress:");
            ImGui::SameLine();
            ImGui::ProgressBar(pct, ImVec2(-1, 0), buf);
        }
    }
}

// ── Mini-map ──────────────────────────────────────────────────────────────────

void PegboardPanel::drawMiniMap(EditorState& state) {
    int tx = state.pegboardManager.tileCountX();
    int ty = state.pegboardManager.tileCountY();
    if (tx <= 0 || ty <= 0) return;

    ImGui::Separator();
    ImGui::Text("Board Map");

    float cellSize = 22.0f;
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

            // Background colour based on fill ratio
            const auto& t = state.pegboardManager.tile(idx);
            int beads = PegboardManager::countBeads(t, state.beadGrid);
            ImU32 bgCol;
            if (beads == 0) {
                bgCol = IM_COL32(60, 60, 60, 255);
            } else {
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

    // ── Per-colour breakdown for current board ────────────────────────────────
    {
        const auto& t = state.pegboardManager.tile(state.currentBoard);
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
            ImGui::Separator();
            ImGui::Text("Board colours: %d", static_cast<int>(used.size()));
            if (ImGui::BeginChild("##boardColours", ImVec2(0, 0),
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
}
