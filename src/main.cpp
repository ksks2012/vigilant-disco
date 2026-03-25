#include "app/window.h"
#include "app/config.h"
#include "app/editor_state.h"
#include "app/control_panel.h"
#include "app/canvas_panel.h"
#include "logging/logger.h"
#include "logging/spdlog_logger.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <algorithm>

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

    // ── Editor state & UI panels ──────────────────────────────────────────────
    EditorState  state;
    ControlPanel controlPanel;
    CanvasPanel  canvasPanel;

    // ── Discover palette files in etc/palettes/ ───────────────────────────────
    {
        namespace fs = std::filesystem;
        const std::string palettesDir = "etc/palettes";
        if (fs::is_directory(palettesDir)) {
            for (const auto& entry : fs::directory_iterator(palettesDir)) {
                if (entry.path().extension() == ".json") {
                    // Try to read the "brand" field for the display label
                    std::string label = entry.path().stem().string();
                    try {
                        std::ifstream f(entry.path());
                        auto j = nlohmann::json::parse(f);
                        if (j.contains("brand") && j["brand"].is_string()) {
                            label = j["brand"].get<std::string>();
                        }
                        if (j.contains("size") && j["size"].is_string()) {
                            label += " — " + j["size"].get<std::string>();
                        }
                        if (j.contains("palette") && j["palette"].is_array()) {
                            label += " (" + std::to_string(j["palette"].size()) + ")";
                        }
                    } catch (...) {
                        // Fall back to filename-based label
                    }
                    state.paletteFiles.push_back({ label, entry.path().string() });
                }
            }
            // Sort alphabetically by label for consistent order
            std::sort(state.paletteFiles.begin(), state.paletteFiles.end(),
                      [](const PaletteFileEntry& a, const PaletteFileEntry& b) {
                          return a.label < b.label;
                      });
        }

        // Fallback: legacy etc/palette.json
        if (state.paletteFiles.empty() && fs::exists("etc/palette.json")) {
            state.paletteFiles.push_back({ "Default", "etc/palette.json" });
        }
    }

    // Load the first palette file (or legacy file if no palettes/ dir)
    if (!state.paletteFiles.empty()) {
        state.palette.loadFromFile(state.paletteFiles[0].path);
        state.paletteFileIndex = 0;
    } else {
        state.palette.loadFromFile("etc/palette.json");
    }
    state.beadGrid.resize(state.gridCols, state.gridRows);

    LOG_INFO("Main", "Perler Bead Simulator started");

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (!window.shouldClose()) {
        window.pollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ── UI panels ─────────────────────────────────────────────────────────
        controlPanel.draw(state);
        canvasPanel.draw(state, window);

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
