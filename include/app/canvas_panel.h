#ifndef CANVAS_PANEL_H
#define CANVAS_PANEL_H

#include "app/layout.h"

struct EditorState;

// Draws the canvas window and handles keyboard shortcuts + mouse interaction.
class CanvasPanel {
public:
    void draw(EditorState& state, const LayoutRect& rect);

private:
    void handleShortcuts(EditorState& state);
    void handleMouseInteraction(EditorState& state);
    void drawCachedTexture(EditorState& state);
    void drawPegboardOverlay(EditorState& state);
    void drawProgressOverlay(EditorState& state);
};

#endif // CANVAS_PANEL_H
