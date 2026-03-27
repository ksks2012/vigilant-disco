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

    // ── Input queries (valid after begin()) ───────────────────────────────────
    // Returns true if the left mouse button was clicked this frame on the canvas
    bool isClicked() const { return clicked_; }
    // Returns true if the left mouse button is held and dragging on the canvas
    bool isDragging() const { return leftDragging_; }
    // World-space position of the mouse (valid when hovered)
    ImVec2 mouseWorldPos() const { return mouseWorld_; }
    // Returns true if the canvas is hovered
    bool isHovered() const { return hovered_; }

    // Zoom limits
    void setZoomRange(float minZoom, float maxZoom) { minZoom_ = minZoom; maxZoom_ = maxZoom; }

    // Reset view to centre the given world rectangle
    void centreView(float worldWidth, float worldHeight);

    // Focus the view on a specific world-space rectangle (cx, cy = centre).
    void focusRect(float cx, float cy, float width, float height);

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
    bool clicked_  = false;
    bool leftDragging_ = false;
    bool hovered_  = false;
    ImVec2 mouseWorld_ = {0, 0};
};

#endif // CANVAS_H
