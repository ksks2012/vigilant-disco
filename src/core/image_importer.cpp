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
                                   BeadGrid& outGrid,
                                   SamplingMethod sampling) {
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
    float scaleX = static_cast<float>(imgW) / static_cast<float>(targetCols);
    float scaleY = static_cast<float>(imgH) / static_cast<float>(targetRows);

    for (int row = 0; row < targetRows; ++row) {
        for (int col = 0; col < targetCols; ++col) {
            int r, g, b;

            if (sampling == SamplingMethod::AreaAverage) {
                // Average all source pixels covered by this bead cell
                int x0 = static_cast<int>(col * scaleX);
                int y0 = static_cast<int>(row * scaleY);
                int x1 = std::min(static_cast<int>((col + 1) * scaleX), imgW);
                int y1 = std::min(static_cast<int>((row + 1) * scaleY), imgH);
                // Ensure at least one pixel is sampled
                if (x1 <= x0) x1 = x0 + 1;
                if (y1 <= y0) y1 = y0 + 1;

                long sumR = 0, sumG = 0, sumB = 0;
                int count = 0;
                for (int sy = y0; sy < y1; ++sy) {
                    for (int sx = x0; sx < x1; ++sx) {
                        int idx = (sy * imgW + sx) * 3;
                        sumR += data[idx + 0];
                        sumG += data[idx + 1];
                        sumB += data[idx + 2];
                        ++count;
                    }
                }
                r = static_cast<int>(sumR / count);
                g = static_cast<int>(sumG / count);
                b = static_cast<int>(sumB / count);
            } else {
                // Point-sample: take the centre pixel of the source region
                int sx = std::clamp(static_cast<int>((col + 0.5f) * scaleX), 0, imgW - 1);
                int sy = std::clamp(static_cast<int>((row + 0.5f) * scaleY), 0, imgH - 1);
                int idx = (sy * imgW + sx) * 3;
                r = data[idx + 0];
                g = data[idx + 1];
                b = data[idx + 2];
            }

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
