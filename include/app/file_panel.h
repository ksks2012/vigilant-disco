#ifndef FILE_PANEL_H
#define FILE_PANEL_H

#include "app/layout.h"

struct EditorState;

// Left-lower panel: project save/load, image import, and export.
class FilePanel {
public:
    void draw(EditorState& state, const LayoutRect& rect);

private:
    void drawProjectSection(EditorState& state);
    void drawImportSection(EditorState& state);
    void drawExportSection(EditorState& state);
};

#endif // FILE_PANEL_H
