#ifndef IMAGE_IMPORTER_H
#define IMAGE_IMPORTER_H

#include "core/bead_grid.h"
#include "core/palette.h"
#include <string>

// Result of an image import operation.
struct ImportResult {
    bool        success = false;
    std::string message;           // human-readable status / error
    int         srcWidth  = 0;     // original image dimensions
    int         srcHeight = 0;
};

// Down-sampling algorithm used during image import.
enum class SamplingMethod {
    PointSample,   // Centre-pixel (nearest-neighbour)
    AreaAverage    // Average all pixels in the source region
};

// Loads an image file, down-samples it to the requested grid width
// (maintaining aspect ratio), and maps each pixel to the nearest
// palette colour via squared Euclidean distance in RGB space.
class ImageImporter {
public:
    // Import an image and fill the bead grid.
    //   path        – file path (PNG, JPG, BMP, etc. via stb_image)
    //   targetCols  – desired grid width in beads
    //   palette     – colour palette for nearest-neighbour matching
    //   outGrid     – bead grid that will be resized and filled
    //   sampling    – down-sampling algorithm
    //
    // The grid height is computed automatically to preserve the image
    // aspect ratio.
    static ImportResult import(const std::string& path,
                               int targetCols,
                               const Palette& palette,
                               BeadGrid& outGrid,
                               SamplingMethod sampling = SamplingMethod::AreaAverage,
                               ColorMatchMethod colorMatch = ColorMatchMethod::EuclideanRGB);
};

#endif // IMAGE_IMPORTER_H
