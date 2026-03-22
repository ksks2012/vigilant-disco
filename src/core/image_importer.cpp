#include "core/image_importer.h"
#include "logging/logger.h"

// stb_image implementation (compiled once here)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cmath>

ImportResult ImageImporter::import(const std::string& path,
                                   int targetCols,
                                   const Palette& palette,
                                   BeadGrid& outGrid) {
    ImportResult result;

    // ── Load image ────────────────────────────────────────────────────────────
    int imgW = 0, imgH = 0, channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &imgW, &imgH, &channels, 3);
    if (!data) {
        result.message = std::string("Failed to load image: ") + stbi_failure_reason();
        LOG_ERROR("ImageImporter", result.message);
        return result;
    }

    result.srcWidth  = imgW;
    result.srcHeight = imgH;

    LOG_INFO("ImageImporter", "Loaded " + path + " (" +
             std::to_string(imgW) + "x" + std::to_string(imgH) +
             ", " + std::to_string(channels) + " ch)");

    // ── Compute grid dimensions (preserve aspect ratio) ───────────────────────
    targetCols = std::clamp(targetCols, 1, 256);
    float aspect = static_cast<float>(imgH) / static_cast<float>(imgW);
    int targetRows = std::max(1, static_cast<int>(std::round(targetCols * aspect)));

    outGrid.resize(targetCols, targetRows);

    // ── Down-sample + nearest-colour matching ─────────────────────────────────
    // For each bead cell, sample the centre pixel of the corresponding region.
    // This is a simple point-sampling (nearest-neighbour) approach; the region
    // size is (imgW / targetCols) x (imgH / targetRows).
    float scaleX = static_cast<float>(imgW) / static_cast<float>(targetCols);
    float scaleY = static_cast<float>(imgH) / static_cast<float>(targetRows);

    for (int row = 0; row < targetRows; ++row) {
        for (int col = 0; col < targetCols; ++col) {
            // Centre of the source region for this bead
            int sx = std::clamp(static_cast<int>((col + 0.5f) * scaleX), 0, imgW - 1);
            int sy = std::clamp(static_cast<int>((row + 0.5f) * scaleY), 0, imgH - 1);

            int pixelIdx = (sy * imgW + sx) * 3;
            int r = data[pixelIdx + 0];
            int g = data[pixelIdx + 1];
            int b = data[pixelIdx + 2];

            uint8_t colorIdx = static_cast<uint8_t>(palette.matchNearest(r, g, b));
            outGrid.set(col, row, colorIdx);
        }
    }

    stbi_image_free(data);

    result.success = true;
    result.message = "Imported as " + std::to_string(targetCols) + "x" +
                     std::to_string(targetRows) + " grid";
    LOG_INFO("ImageImporter", result.message);
    return result;
}
