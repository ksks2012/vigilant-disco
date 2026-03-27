#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

struct EditorState;

// Draws the left-side "Controls" window with all editor UI sections:
// Project, Import, Export, Grid Size, Tools, Palette, View Info, Help.
class ControlPanel {
public:
    void draw(EditorState& state);

private:
    void drawProjectSection(EditorState& state);
    void drawImportSection(EditorState& state);
    void drawExportSection(EditorState& state);
    void drawGridSizeSection(EditorState& state);
    void drawPegboardSection(EditorState& state);
    void drawToolsSection(EditorState& state);
    void drawPaletteSection(EditorState& state);
    void drawViewInfoSection(EditorState& state);
    void drawHelpSection();
};

#endif // CONTROL_PANEL_H
