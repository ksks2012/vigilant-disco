#ifndef LAYOUT_H
#define LAYOUT_H

#include <imgui.h>

// A rectangular region in screen coordinates.
struct LayoutRect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    ImVec2 pos()  const { return ImVec2(x, y); }
    ImVec2 size() const { return ImVec2(w, h); }
};

// Computed layout for the entire application.
// All coordinates are in screen pixels, recalculated every frame.
struct AppLayout {
    LayoutRect toolsPanel;      // left-upper
    LayoutRect filePanel;       // left-lower
    LayoutRect canvasPanel;     // centre
    LayoutRect pegboardPanel;   // right-upper
    LayoutRect statsPanel;      // right-lower
};

// Computes a responsive layout based on the current window size.
// Side panels use proportional widths clamped to reasonable bounds.
class LayoutManager {
public:
    // Recalculate all panel rectangles for the given window dimensions.
    static AppLayout compute(float windowW, float windowH) {
        AppLayout lay;

        // ── Side panel widths (proportional, with min/max clamps) ─────────
        float leftW  = windowW * 0.22f;
        leftW = clamp(leftW, 220.0f, 340.0f);

        float rightW = windowW * 0.17f;
        rightW = clamp(rightW, 180.0f, 280.0f);

        float centreW = windowW - leftW - rightW;
        if (centreW < 200.0f) {
            // If centre is too narrow, shrink side panels equally
            float deficit = 200.0f - centreW;
            float half = deficit * 0.5f;
            leftW  -= half;
            rightW -= half;
            centreW = 200.0f;
        }

        // ── Vertical split (50/50 for left and right columns) ─────────────
        float leftTopH    = windowH * 0.50f;
        float leftBottomH = windowH - leftTopH;

        float rightTopH    = windowH * 0.50f;
        float rightBottomH = windowH - rightTopH;

        // ── Assign rectangles ─────────────────────────────────────────────
        lay.toolsPanel    = { 0.0f,            0.0f,      leftW,   leftTopH    };
        lay.filePanel     = { 0.0f,            leftTopH,  leftW,   leftBottomH };

        lay.canvasPanel   = { leftW,           0.0f,      centreW, windowH     };

        float rightX = leftW + centreW;
        lay.pegboardPanel = { rightX,          0.0f,      rightW,  rightTopH    };
        lay.statsPanel    = { rightX,          rightTopH, rightW,  rightBottomH };

        return lay;
    }

private:
    static float clamp(float v, float lo, float hi) {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }
};

#endif // LAYOUT_H
