#ifndef EXPORTER_H
#define EXPORTER_H

#include "core/bead_grid.h"
#include "core/palette.h"
#include <string>

// Result of an export operation.
struct ExportResult {
    bool        success = false;
    std::string message;
};

// PNG export style
enum class PngStyle {
    Flat,   // Simple colour blocks with grid lines
    Bead    // Circular beads with highlight / shadow (3D-ish)
};

// Handles exporting the bead grid to various file formats.
class Exporter {
public:
    // Export the grid as a PNG image.
    //   beadPx – size of each bead in pixels (e.g. 20)
    //   style  – Flat or Bead
    static ExportResult exportPng(const std::string& path,
                                  const BeadGrid& grid,
                                  const Palette& palette,
                                  int beadPx = 20,
                                  PngStyle style = PngStyle::Bead);

    // Export a CSV bead count summary.
    // Columns: Index, Name, R, G, B, Count
    static ExportResult exportCsv(const std::string& path,
                                  const BeadGrid& grid,
                                  const Palette& palette);
};

#endif // EXPORTER_H
