#include "app/stats_panel.h"
#include "app/editor_state.h"
#include "app/window.h"

#include <imgui.h>
#include <algorithm>
#include <vector>

// ── Main draw ─────────────────────────────────────────────────────────────────

void StatsPanel::draw(EditorState& state, const Window& window) {
    // Position the panel at the bottom-right of the window
    float panelWidth = 220.0f;
    float winW = static_cast<float>(window.getWidth());
    float winH = static_cast<float>(window.getHeight());
    float topH = winH * 0.50f;
    float panelH = winH - topH;

    ImGui::SetNextWindowPos(ImVec2(winW - panelWidth, topH), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelH), ImGuiCond_Always);

    ImGui::Begin("Bead Statistics", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse);

    // ── Count beads per colour index ──────────────────────────────────────────
    const auto& cells = state.beadGrid.cells();
    int paletteSize = state.palette.size();

    // counts[i] = number of beads with colour index i
    std::vector<int> counts(paletteSize, 0);
    int totalBeads = 0;

    for (uint8_t idx : cells) {
        if (idx > 0 && idx < paletteSize) {
            counts[idx]++;
            totalBeads++;
        }
    }

    // Grid dimensions info
    ImGui::Text("Grid: %d x %d", state.beadGrid.cols(), state.beadGrid.rows());
    ImGui::Text("Total beads: %d", totalBeads);
    int emptyCount = static_cast<int>(cells.size()) - totalBeads;
    ImGui::Text("Empty cells: %d", emptyCount);
    ImGui::Separator();

    // ── Build sorted list of used colours ─────────────────────────────────────
    struct ColourStat {
        int index;
        int count;
    };
    std::vector<ColourStat> usedColours;
    for (int i = 1; i < paletteSize; ++i) {
        if (counts[i] > 0) {
            usedColours.push_back({ i, counts[i] });
        }
    }

    // Sort by count descending
    std::sort(usedColours.begin(), usedColours.end(),
              [](const ColourStat& a, const ColourStat& b) {
                  return a.count > b.count;
              });

    ImGui::Text("Colours used: %d / %d", static_cast<int>(usedColours.size()),
                paletteSize - 1);
    ImGui::Separator();

    // ── Scrollable colour table ───────────────────────────────────────────────
    if (ImGui::BeginChild("ColourStats", ImVec2(0, 0), ImGuiChildFlags_None)) {
        // 3-column table: [Swatch + Name] [Count] [%]
        ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg
                                   | ImGuiTableFlags_ScrollY
                                   | ImGuiTableFlags_SizingStretchProp;
        if (ImGui::BeginTable("##stats_table", 3, tableFlags)) {
            ImGui::TableSetupColumn("Colour",  ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("Count",   ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("%",       ImGuiTableColumnFlags_WidthFixed, 48.0f);

            float swatchSize = 14.0f;

            for (const auto& cs : usedColours) {
                ImU32 col = state.palette.color(cs.index);
                ImVec4 colVec = ImGui::ColorConvertU32ToFloat4(col);

                float pct = (totalBeads > 0)
                    ? (static_cast<float>(cs.count) / totalBeads) * 100.0f
                    : 0.0f;

                ImGui::TableNextRow();

                // Column 0: colour swatch + name
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID(cs.index);
                ImGui::ColorButton("##sw", colVec,
                                   ImGuiColorEditFlags_NoTooltip |
                                   ImGuiColorEditFlags_NoBorder,
                                   ImVec2(swatchSize, swatchSize));
                ImGui::PopID();
                ImGui::SameLine();
                ImGui::TextUnformatted(state.palette.name(cs.index));

                // Column 1: count
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%d", cs.count);

                // Column 2: percentage
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.1f%%", pct);
            }

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

    ImGui::End();
}
