#ifndef TOOLS_PANEL_H
#define TOOLS_PANEL_H

struct EditorState;
class Window;

// Left-upper panel: editing tools, brush settings, undo/redo, and palette.
class ToolsPanel {
public:
    void draw(EditorState& state, const Window& window);

private:
    void drawToolsSection(EditorState& state);
    void drawPaletteSection(EditorState& state);
    void drawViewInfoSection(EditorState& state);
};

#endif // TOOLS_PANEL_H
