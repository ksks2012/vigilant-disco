#ifndef STATS_PANEL_H
#define STATS_PANEL_H

#include "app/layout.h"

struct EditorState;

// Draws the right-side "Bead Statistics" window showing colour usage counts.
class StatsPanel {
public:
    void draw(EditorState& state, const LayoutRect& rect);
};

#endif // STATS_PANEL_H
