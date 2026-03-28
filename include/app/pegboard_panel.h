#ifndef PEGBOARD_PANEL_H
#define PEGBOARD_PANEL_H

#include "app/layout.h"

struct EditorState;

// Right-upper panel: pegboard settings, navigation, and mini-map.
class PegboardPanel {
public:
    void draw(EditorState& state, const LayoutRect& rect);

private:
    void drawSettingsSection(EditorState& state);
    void drawNavigationSection(EditorState& state);
    void drawMiniMap(EditorState& state);
};

#endif // PEGBOARD_PANEL_H
