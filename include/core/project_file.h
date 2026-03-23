#ifndef PROJECT_FILE_H
#define PROJECT_FILE_H

#include "core/bead_grid.h"
#include "core/palette.h"
#include <string>

// Result of a project file operation.
struct ProjectFileResult {
    bool        success = false;
    std::string message;
};

// Handles saving and loading .pin project files (JSON format).
//
// File format (JSON):
// {
//   "version": 1,
//   "grid": { "cols": N, "rows": M, "cells": [0,1,3,...] },
//   "palette": [ {"name":"Black","color":[0,0,0]}, ... ]
// }
class ProjectFile {
public:
    // Save the current grid and palette to a .pin file.
    static ProjectFileResult save(const std::string& path,
                                  const BeadGrid& grid,
                                  const Palette& palette);

    // Load a .pin file into the grid and palette.
    // On success the grid will be resized and filled, and the palette
    // will be replaced with the one stored in the file.
    static ProjectFileResult load(const std::string& path,
                                  BeadGrid& grid,
                                  Palette& palette);
};

#endif // PROJECT_FILE_H
