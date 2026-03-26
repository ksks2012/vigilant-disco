#ifndef STATS_PANEL_H
#define STATS_PANEL_H

struct EditorState;
class Window;

// Draws the right-side "Bead Statistics" window showing colour usage counts.
class StatsPanel {
public:
    void draw(EditorState& state, const Window& window);
};

#endif // STATS_PANEL_H
