#ifndef CANVAS_PANEL_H
#define CANVAS_PANEL_H

struct EditorState;
class Window;

// Draws the canvas window and handles keyboard shortcuts + mouse interaction.
class CanvasPanel {
public:
    void draw(EditorState& state, const Window& window);

private:
    void handleShortcuts(EditorState& state);
    void handleMouseInteraction(EditorState& state);
    void drawCachedTexture(EditorState& state);
};

#endif // CANVAS_PANEL_H
