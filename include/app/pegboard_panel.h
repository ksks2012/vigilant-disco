#ifndef PEGBOARD_PANEL_H
#define PEGBOARD_PANEL_H

struct EditorState;
class Window;

// Right-upper panel: pegboard settings, navigation, and mini-map.
class PegboardPanel {
public:
    void draw(EditorState& state, const Window& window);

private:
    void drawSettingsSection(EditorState& state);
    void drawNavigationSection(EditorState& state);
    void drawMiniMap(EditorState& state);
};

#endif // PEGBOARD_PANEL_H
