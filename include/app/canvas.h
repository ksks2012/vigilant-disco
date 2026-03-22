#ifndef CANVAS_H
#define CANVAS_H

#include <imgui.h>

// 2D canvas with zoom and pan support.
// Manages the transformation between screen pixels and world (grid) coordinates.
class Canvas {
public:
    Canvas();

    // Call once per frame before drawing. Sets up the canvas region and
    // processes mouse input for pan/zoom. Returns true if the canvas is
    // visible (not clipped).
    bool begin(const char* label = "##canvas");
    void end();

    // ── Coordinate transforms ─────────────────────────────────────────────────
    // World  → Screen
    ImVec2 worldToScreen(float wx, float wy) const;
    // Screen → World
    ImVec2 screenToWorld(float sx, float sy) const;

    // Scale factor: how many screen pixels per world unit
    float scale() const { return zoom_; }

    // Current view offset in world units
    ImVec2 offset() const { return offset_; }

    // Canvas top-left corner in screen coordinates
    ImVec2 origin() const { return canvasOrigin_; }
    // Canvas size in screen pixels
    ImVec2 size()   const { return canvasSize_; }

    // The ImDrawList for the canvas area
    ImDrawList* drawList() const { return drawList_; }

    // Zoom limits
    void setZoomRange(float minZoom, float maxZoom) { minZoom_ = minZoom; maxZoom_ = maxZoom; }

    // Reset view to centre the given world rectangle
    void centreView(float worldWidth, float worldHeight);

private:
    // View state
    float  zoom_   = 20.0f;  // pixels per world unit
    ImVec2 offset_ = {0.0f, 0.0f}; // world-space offset (pan)

    float minZoom_ = 4.0f;
    float maxZoom_ = 80.0f;

    // Per-frame cached values
    ImVec2      canvasOrigin_ = {0, 0};
    ImVec2      canvasSize_   = {0, 0};
    ImDrawList* drawList_     = nullptr;

    // Interaction state
    bool dragging_ = false;
};

#endif // CANVAS_H
