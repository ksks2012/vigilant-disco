#include "app/canvas.h"
#include <algorithm>
#include <cmath>

Canvas::Canvas() = default;

bool Canvas::begin(const char* label) {
    // Use the entire available region in the current ImGui window
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x <= 0.0f || avail.y <= 0.0f)
        return false;

    canvasOrigin_ = ImGui::GetCursorScreenPos();
    canvasSize_   = avail;

    // Invisible button fills the canvas area and captures input
    ImGui::InvisibleButton(label, canvasSize_,
                           ImGuiButtonFlags_MouseButtonLeft |
                           ImGuiButtonFlags_MouseButtonRight |
                           ImGuiButtonFlags_MouseButtonMiddle);
    const bool isHovered = ImGui::IsItemHovered();
    const bool isActive  = ImGui::IsItemActive();

    drawList_ = ImGui::GetWindowDrawList();

    // ── Pan (middle-click or right-click drag) ────────────────────────────────
    if (isActive && (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f) ||
                     ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        // Convert screen-pixel delta to world-unit delta
        offset_.x -= delta.x / zoom_;
        offset_.y -= delta.y / zoom_;
    }

    // ── Zoom (scroll wheel, centred on mouse) ─────────────────────────────────
    if (isHovered) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            // Mouse position in world coords BEFORE zoom
            ImVec2 mouseScreen = ImGui::GetIO().MousePos;
            ImVec2 mouseWorldBefore = screenToWorld(mouseScreen.x, mouseScreen.y);

            // Apply zoom
            float factor = (wheel > 0.0f) ? 1.15f : (1.0f / 1.15f);
            zoom_ = std::clamp(zoom_ * factor, minZoom_, maxZoom_);

            // Mouse position in world coords AFTER zoom
            ImVec2 mouseWorldAfter = screenToWorld(mouseScreen.x, mouseScreen.y);

            // Adjust offset so the world point under the mouse stays fixed
            offset_.x -= (mouseWorldAfter.x - mouseWorldBefore.x);
            offset_.y -= (mouseWorldAfter.y - mouseWorldBefore.y);
        }
    }

    // Clip drawing to canvas bounds
    drawList_->PushClipRect(canvasOrigin_,
                            ImVec2(canvasOrigin_.x + canvasSize_.x,
                                   canvasOrigin_.y + canvasSize_.y),
                            true);

    return true;
}

void Canvas::end() {
    if (drawList_)
        drawList_->PopClipRect();
}

ImVec2 Canvas::worldToScreen(float wx, float wy) const {
    float sx = canvasOrigin_.x + (wx - offset_.x) * zoom_;
    float sy = canvasOrigin_.y + (wy - offset_.y) * zoom_;
    return ImVec2(sx, sy);
}

ImVec2 Canvas::screenToWorld(float sx, float sy) const {
    float wx = (sx - canvasOrigin_.x) / zoom_ + offset_.x;
    float wy = (sy - canvasOrigin_.y) / zoom_ + offset_.y;
    return ImVec2(wx, wy);
}

void Canvas::centreView(float worldWidth, float worldHeight) {
    // Fit the world rectangle into the canvas with some padding
    if (canvasSize_.x <= 0.0f || canvasSize_.y <= 0.0f) return;

    float scaleX = canvasSize_.x / worldWidth;
    float scaleY = canvasSize_.y / worldHeight;
    zoom_ = std::min(scaleX, scaleY) * 0.9f; // 90% fill
    zoom_ = std::clamp(zoom_, minZoom_, maxZoom_);

    // Centre offset
    offset_.x = worldWidth  * 0.5f - (canvasSize_.x * 0.5f) / zoom_;
    offset_.y = worldHeight * 0.5f - (canvasSize_.y * 0.5f) / zoom_;
}
