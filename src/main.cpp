#include "app/window.h"
#include "app/config.h"
#include "app/canvas.h"
#include "logging/logger.h"
#include "logging/spdlog_logger.h"
#include "core/bead_grid.h"
#include "core/palette.h"
#include "core/image_importer.h"
#include "rendering/grid_renderer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <memory>
#include <string>
#include <cmath>

int main(int /*argc*/, char* /*argv*/[]) {
    // ── Logger setup ──────────────────────────────────────────────────────────
    auto logger = std::make_shared<SpdlogLogger>();
    GlobalLogger::set(logger);

    // ── Config ────────────────────────────────────────────────────────────────
    Config config;
    try {
        config.loadFromFile("etc/config.json");
    } catch (const ConfigError& e) {
        LOG_ERROR("Main", std::string("Config error: ") + e.what());
        return 1;
    }

    LogLevel logLevel = static_cast<LogLevel>(config.logger_level);
    logger->set_level(logLevel);

    // ── Window + OpenGL context ───────────────────────────────────────────────
    Window window(config.window.title, config.window.width, config.window.height);

    // ── ImGui setup ───────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window.getHandle(), true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Background clear colour (soft light grey)
    ImVec4 clear_color = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);

    // ── Grid data & renderer ──────────────────────────────────────────────────
    Canvas canvas;
    BeadGrid beadGrid;
    GridRenderer gridRenderer;
    Palette palette;
    palette.loadFromFile("etc/palette.json");

    int gridCols = 29;
    int gridRows = 29;
    beadGrid.resize(gridCols, gridRows);

    int selectedColor = 1; // current palette index for painting (default: Black)
    bool viewCentred = false;

    // ── Image import state ────────────────────────────────────────────────────
    char importPath[512] = "";
    int  importWidth = 29;
    std::string importStatus;

    LOG_INFO("Main", "Perler Bead Simulator started");

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (!window.shouldClose()) {
        window.pollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ── Control panel ─────────────────────────────────────────────────────
        {
            ImGui::Begin("Controls");
            ImGui::Text("FPS: %.1f (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);

            // ── Import image ──────────────────────────────────────────────────
            ImGui::Separator();
            ImGui::Text("Import Image");
            ImGui::InputText("File Path", importPath, sizeof(importPath));
            ImGui::SliderInt("Width (beads)", &importWidth, 1, 256);

            if (ImGui::Button("Import")) {
                std::string filePath(importPath);
                if (!filePath.empty()) {
                    auto result = ImageImporter::import(filePath, importWidth,
                                                        palette, beadGrid);
                    importStatus = result.message;
                    if (result.success) {
                        gridCols = beadGrid.cols();
                        gridRows = beadGrid.rows();
                        viewCentred = false; // re-centre view for new grid
                    }
                } else {
                    importStatus = "Please enter a file path";
                }
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Supports PNG, JPG, BMP, TGA, GIF, PSD, HDR, PIC");
            }

            if (!importStatus.empty()) {
                ImGui::TextWrapped("%s", importStatus.c_str());
            }

            // ── Grid size ─────────────────────────────────────────────────────
            ImGui::Separator();
            ImGui::Text("Grid Size");
            bool sizeChanged = false;
            sizeChanged |= ImGui::SliderInt("Columns", &gridCols, 1, 100);
            sizeChanged |= ImGui::SliderInt("Rows",    &gridRows, 1, 100);
            if (sizeChanged) {
                beadGrid.resize(gridCols, gridRows);
            }

            if (ImGui::Button("Clear Grid")) {
                beadGrid.clear();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset View")) {
                viewCentred = false;
            }

            // ── Colour palette ────────────────────────────────────────────────
            ImGui::Separator();
            ImGui::Text("Palette");
            ImGui::Text("Selected: %s", palette.name(selectedColor));

            // Draw palette buttons in a wrapped row
            float buttonSize = 28.0f;
            float panelWidth = ImGui::GetContentRegionAvail().x;
            int buttonsPerRow = std::max(1, static_cast<int>(panelWidth / (buttonSize + 4.0f)));

            for (int i = 0; i < palette.size(); ++i) {
                ImU32 col = palette.color(i);
                // Convert ImU32 to ImVec4 using ImGui's built-in conversion
                ImVec4 colVec = ImGui::ColorConvertU32ToFloat4(col);

                ImGui::PushID(i);
                // Highlight the selected colour
                bool isSelected = (i == selectedColor);
                if (isSelected) {
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 3.0f);
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                }

                if (ImGui::ColorButton(palette.name(i), colVec,
                                       ImGuiColorEditFlags_NoTooltip,
                                       ImVec2(buttonSize, buttonSize))) {
                    selectedColor = i;
                }

                if (isSelected) {
                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();
                }

                // Tooltip on hover
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s (index %d)", palette.name(i), i);
                }

                ImGui::PopID();

                // Wrap to next row
                if ((i + 1) % buttonsPerRow != 0 && i + 1 < palette.size()) {
                    ImGui::SameLine();
                }
            }

            // ── View info ─────────────────────────────────────────────────────
            ImGui::Separator();
            ImGui::Text("Zoom: %.1f px/unit", canvas.scale());

            if (canvas.isHovered()) {
                ImVec2 mw = canvas.mouseWorldPos();
                int hoverCol = static_cast<int>(std::floor(mw.x));
                int hoverRow = static_cast<int>(std::floor(mw.y));
                if (beadGrid.inBounds(hoverCol, hoverRow)) {
                    uint8_t ci = beadGrid.get(hoverCol, hoverRow);
                    ImGui::Text("Hover: (%d, %d) = %s", hoverCol, hoverRow,
                                palette.name(ci));
                }
            }

            // ── Help ──────────────────────────────────────────────────────────
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Controls:");
            ImGui::BulletText("Left-click / drag: Paint bead");
            ImGui::BulletText("Scroll: Zoom in/out");
            ImGui::BulletText("Right / Middle drag: Pan");
            ImGui::BulletText("Import: Load image into grid");

            ImGui::End();
        }

        // ── Canvas ────────────────────────────────────────────────────────────
        {
            ImGui::SetNextWindowPos(ImVec2(280, 0), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(
                ImVec2(static_cast<float>(window.getWidth()) - 280.0f,
                       static_cast<float>(window.getHeight())),
                ImGuiCond_FirstUseEver);

            ImGui::Begin("Canvas", nullptr,
                         ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse);

            if (canvas.begin()) {
                // Centre the view on the first valid frame
                if (!viewCentred) {
                    canvas.centreView(static_cast<float>(beadGrid.cols()),
                                      static_cast<float>(beadGrid.rows()));
                    viewCentred = true;
                }

                // ── Mouse picking: paint on click or drag ─────────────────────
                if (canvas.isClicked() || canvas.isDragging()) {
                    ImVec2 mw = canvas.mouseWorldPos();
                    int col = static_cast<int>(std::floor(mw.x));
                    int row = static_cast<int>(std::floor(mw.y));
                    if (beadGrid.inBounds(col, row)) {
                        beadGrid.set(col, row, static_cast<uint8_t>(selectedColor));
                    }
                }

                gridRenderer.draw(canvas, beadGrid, palette);
                canvas.end();
            }

            ImGui::End();
        }

        // ── Render ────────────────────────────────────────────────────────────
        ImGui::Render();
        glViewport(0, 0, window.getWidth(), window.getHeight());
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.swapBuffers();
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    LOG_INFO("Main", "Shutdown complete");
    return 0;
}
