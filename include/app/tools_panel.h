#ifndef TOOLS_PANEL_H
#define TOOLS_PANEL_H

#include "app/layout.h"

struct EditorState;

// Left-upper panel: editing tools, brush settings, undo/redo, and palette.
class ToolsPanel {
public:
    void draw(EditorState& state, const LayoutRect& rect);

private:
    void drawToolsSection(EditorState& state);
    void drawPaletteSection(EditorState& state);
    void drawViewInfoSection(EditorState& state);
};

#endif // TOOLS_PANEL_H
