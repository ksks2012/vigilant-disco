#include "core/bead_grid.h"
#include <algorithm>
#include <queue>
#include <vector>

// ── BeadGrid implementation ───────────────────────────────────────────────────

BeadGrid::BeadGrid() = default;

void BeadGrid::resize(int cols, int rows) {
    cols_ = std::max(1, cols);
    rows_ = std::max(1, rows);
    cells_.assign(static_cast<size_t>(cols_) * rows_, 0);
}

uint8_t BeadGrid::get(int col, int row) const {
    if (!inBounds(col, row)) return 0;
    return cells_[static_cast<size_t>(row) * cols_ + col];
}

void BeadGrid::set(int col, int row, uint8_t colorIdx) {
    if (!inBounds(col, row)) return;
    cells_[static_cast<size_t>(row) * cols_ + col] = colorIdx;
}

void BeadGrid::clear() {
    std::fill(cells_.begin(), cells_.end(), static_cast<uint8_t>(0));
}

bool BeadGrid::inBounds(int col, int row) const {
    return col >= 0 && col < cols_ && row >= 0 && row < rows_;
}

void BeadGrid::restoreFrom(int cols, int rows, const std::vector<uint8_t>& cells) {
    cols_ = cols;
    rows_ = rows;
    cells_ = cells;
}

void BeadGrid::paintBrush(int col, int row, uint8_t colorIdx, int brushSize) {
    // brushSize 1 = single bead, 2 = 3x3, 3 = 5x5
    int radius = brushSize - 1; // 0, 1, or 2
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            set(col + dx, row + dy, colorIdx);
        }
    }
}

void BeadGrid::floodFill(int col, int row, uint8_t newColorIdx) {
    if (!inBounds(col, row)) return;

    uint8_t targetColor = get(col, row);
    if (targetColor == newColorIdx) return; // already the desired colour

    // Iterative BFS to avoid stack overflow on large grids
    std::queue<std::pair<int, int>> q;
    q.push({ col, row });
    set(col, row, newColorIdx);

    // 4-connected neighbours (up, down, left, right)
    static constexpr int dx[] = { 0, 0, -1, 1 };
    static constexpr int dy[] = { -1, 1, 0, 0 };

    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();

        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];
            if (inBounds(nx, ny) && get(nx, ny) == targetColor) {
                set(nx, ny, newColorIdx);
                q.push({ nx, ny });
            }
        }
    }
}

// ── Background removal ────────────────────────────────────────────────────────

// Helper: extract RGB components from a palette ImU32 colour.
static void paletteRGB(const Palette& palette, int idx, int& r, int& g, int& b) {
    ImU32 c = palette.color(idx);
    r = static_cast<int>((c >> IM_COL32_R_SHIFT) & 0xFF);
    g = static_cast<int>((c >> IM_COL32_G_SHIFT) & 0xFF);
    b = static_cast<int>((c >> IM_COL32_B_SHIFT) & 0xFF);
}

// Squared RGB distance between two palette entries.
static int colourDistSq(const Palette& palette, int idxA, int idxB) {
    int rA, gA, bA, rB, gB, bB;
    paletteRGB(palette, idxA, rA, gA, bA);
    paletteRGB(palette, idxB, rB, gB, bB);
    int dr = rA - rB, dg = gA - gB, db = bA - bB;
    return dr * dr + dg * dg + db * db;
}

int BeadGrid::removeBackground(const Palette& palette, int tolerance) {
    if (cols_ <= 0 || rows_ <= 0) return 0;

    const int tolSq = tolerance * tolerance; // compare squared distances
    int cleared = 0;

    // Snapshot the original colours *before* any clearing so BFS can always
    // refer to what a cell was before it was erased.
    std::vector<uint8_t> origColour(cells_.begin(), cells_.end());

    // visited[row * cols_ + col] — prevent re-visiting
    std::vector<bool> visited(cells_.size(), false);

    // Seed the BFS from every edge cell
    std::queue<std::pair<int, int>> q;

    auto seedEdge = [&](int col, int row) {
        size_t idx = static_cast<size_t>(row) * cols_ + col;
        if (visited[idx]) return;
        visited[idx] = true;
        uint8_t ci = origColour[idx];
        if (ci != 0) {
            // Non-empty edge cell: always treat it as background.
            cells_[idx] = 0;
            ++cleared;
        }
        q.push({ col, row });
    };

    // Top & bottom edges
    for (int c = 0; c < cols_; ++c) {
        seedEdge(c, 0);
        seedEdge(c, rows_ - 1);
    }
    // Left & right edges (skip corners already seeded)
    for (int r = 1; r < rows_ - 1; ++r) {
        seedEdge(0, r);
        seedEdge(cols_ - 1, r);
    }

    // 4-connected neighbours
    static constexpr int dx[] = { 0, 0, -1, 1 };
    static constexpr int dy[] = { -1, 1, 0, 0 };

    // BFS: propagate into neighbours whose colour is "close enough" to the
    // source cell that reached them.  We use origColour[] so comparisons
    // are always against the pre-clearing colour.
    //
    // When the source cell was originally empty (index 0) it acts as a
    // transparent conduit: we propagate through it but compare the
    // neighbour's colour against the nearest non-empty background cell
    // that led us here.  To achieve this we store a "reference colour
    // index" per queued cell — the last non-empty colour along the path.
    // For edge seeds the reference is their own original colour (or 0
    // if the edge cell was already empty).
    struct BfsNode { int col, row; uint8_t refIdx; };
    std::queue<BfsNode> bfs;

    // Re-seed with reference colour information.  Clear the plain queue
    // (we consumed all of it) and rebuild with BfsNode.
    // Since we already visited and enqueued everything, just rebuild:
    // reset visited and redo seed — tiny overhead vs grid size.
    std::fill(visited.begin(), visited.end(), false);
    // Note: cells_ already cleared for edge cells above; use origColour.

    // We'll use bfs queue directly now.
    auto seedEdge2 = [&](int col, int row) {
        size_t idx = static_cast<size_t>(row) * cols_ + col;
        if (visited[idx]) return;
        visited[idx] = true;
        bfs.push({ col, row, origColour[idx] });
    };

    for (int c = 0; c < cols_; ++c) {
        seedEdge2(c, 0);
        seedEdge2(c, rows_ - 1);
    }
    for (int r = 1; r < rows_ - 1; ++r) {
        seedEdge2(0, r);
        seedEdge2(cols_ - 1, r);
    }

    while (!bfs.empty()) {
        auto [cx, cy, refIdx] = bfs.front();
        bfs.pop();

        for (int d = 0; d < 4; ++d) {
            int nx = cx + dx[d];
            int ny = cy + dy[d];
            if (!inBounds(nx, ny)) continue;
            size_t ni = static_cast<size_t>(ny) * cols_ + nx;
            if (visited[ni]) continue;

            uint8_t neighbourOrig = origColour[ni];

            if (neighbourOrig == 0) {
                // Already empty — propagate through, keep same reference
                visited[ni] = true;
                bfs.push({ nx, ny, refIdx });
                continue;
            }

            // If we have no non-empty reference along this path, we
            // can't judge colour similarity — don't propagate further.
            if (refIdx == 0) continue;

            int dist = colourDistSq(palette, neighbourOrig, refIdx);
            if (dist <= tolSq) {
                visited[ni] = true;
                cells_[ni] = 0;
                ++cleared;
                // Update reference to *this* cell's original colour so
                // we adapt to gradual colour changes in the background.
                bfs.push({ nx, ny, neighbourOrig });
            }
        }
    }

    return cleared;
}
