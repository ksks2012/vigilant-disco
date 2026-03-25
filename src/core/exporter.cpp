#include "core/exporter.h"
#include "logging/logger.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <vector>
#include <map>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <cstring>

// ── Helper: blend a colour over a pixel with alpha ────────────────────────────
static inline void blendPixel(uint8_t* dst, int r, int g, int b, int a) {
    // dst is RGBA. Simple alpha-over compositing.
    int da = dst[3];
    int dr = dst[0], dg = dst[1], db = dst[2];

    int outA = a + da * (255 - a) / 255;
    if (outA == 0) return;

    dst[0] = static_cast<uint8_t>((r * a + dr * da * (255 - a) / 255) / outA);
    dst[1] = static_cast<uint8_t>((g * a + dg * da * (255 - a) / 255) / outA);
    dst[2] = static_cast<uint8_t>((b * a + db * da * (255 - a) / 255) / outA);
    dst[3] = static_cast<uint8_t>(outA);
}

// ── Helper: set pixel (no blending) ───────────────────────────────────────────
static inline void setPixel(uint8_t* img, int imgW, int x, int y,
                             uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    int idx = (y * imgW + x) * 4;
    img[idx + 0] = r;
    img[idx + 1] = g;
    img[idx + 2] = b;
    img[idx + 3] = a;
}

// ── Helper: draw filled circle with optional highlight/shadow ─────────────────
static void drawBead(uint8_t* img, int imgW, int imgH,
                     int cx, int cy, int radius,
                     int r, int g, int b, bool bead3D) {
    int r2 = radius * radius;

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int dist2 = dx * dx + dy * dy;
            if (dist2 > r2) continue;

            int px = cx + dx;
            int py = cy + dy;
            if (px < 0 || px >= imgW || py < 0 || py >= imgH) continue;

            int fr = r, fg = g, fb = b;

            if (bead3D) {
                // Simple radial shading: lighter towards top-left, darker bottom-right
                float dist = std::sqrt(static_cast<float>(dist2));
                float normDist = dist / static_cast<float>(radius);

                // Light offset towards top-left
                float lightX = static_cast<float>(dx) / radius;
                float lightY = static_cast<float>(dy) / radius;
                float lightFactor = -0.3f * lightX - 0.3f * lightY; // -1..1 range
                lightFactor = std::clamp(lightFactor, -0.4f, 0.5f);

                // Rim darkening
                float rimDarken = normDist * normDist * 0.2f;

                float brightness = 1.0f + lightFactor - rimDarken;
                brightness = std::clamp(brightness, 0.5f, 1.4f);

                fr = std::clamp(static_cast<int>(r * brightness), 0, 255);
                fg = std::clamp(static_cast<int>(g * brightness), 0, 255);
                fb = std::clamp(static_cast<int>(b * brightness), 0, 255);
            }

            setPixel(img, imgW, px, py,
                     static_cast<uint8_t>(fr),
                     static_cast<uint8_t>(fg),
                     static_cast<uint8_t>(fb));
        }
    }
}

// ── PNG export ────────────────────────────────────────────────────────────────

ExportResult Exporter::exportPng(const std::string& path,
                                  const BeadGrid& grid,
                                  const Palette& palette,
                                  int beadPx,
                                  PngStyle style) {
    ExportResult result;

    int cols = grid.cols();
    int rows = grid.rows();
    beadPx = std::clamp(beadPx, 4, 64);

    int imgW = cols * beadPx;
    int imgH = rows * beadPx;

    if (imgW <= 0 || imgH <= 0) {
        result.message = "Grid is empty";
        return result;
    }

    // Allocate RGBA image
    std::vector<uint8_t> img(static_cast<size_t>(imgW) * imgH * 4);

    // Fill with white background
    std::memset(img.data(), 255, img.size());

    bool is3D = (style == PngStyle::Bead);

    // ── Draw board background (light grey for bead style) ─────────────────────
    if (is3D) {
        for (int i = 0; i < imgW * imgH; ++i) {
            img[i * 4 + 0] = 230;
            img[i * 4 + 1] = 230;
            img[i * 4 + 2] = 235;
            img[i * 4 + 3] = 255;
        }
    }

    // ── Draw beads ────────────────────────────────────────────────────────────
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            uint8_t colorIdx = grid.get(col, row);
            ImU32 c = palette.color(colorIdx);

            int r = static_cast<int>((c >> IM_COL32_R_SHIFT) & 0xFF);
            int g = static_cast<int>((c >> IM_COL32_G_SHIFT) & 0xFF);
            int b = static_cast<int>((c >> IM_COL32_B_SHIFT) & 0xFF);

            if (is3D) {
                // Draw circular bead with 3D shading
                int cx = col * beadPx + beadPx / 2;
                int cy = row * beadPx + beadPx / 2;
                int radius = static_cast<int>(beadPx * 0.42f);
                drawBead(img.data(), imgW, imgH, cx, cy, radius, r, g, b, true);
            } else {
                // Flat mode: fill the cell with the colour
                int x0 = col * beadPx;
                int y0 = row * beadPx;
                for (int dy = 0; dy < beadPx; ++dy) {
                    for (int dx = 0; dx < beadPx; ++dx) {
                        setPixel(img.data(), imgW, x0 + dx, y0 + dy,
                                 static_cast<uint8_t>(r),
                                 static_cast<uint8_t>(g),
                                 static_cast<uint8_t>(b));
                    }
                }
            }
        }
    }

    // ── Draw grid lines (flat mode only) ──────────────────────────────────────
    if (!is3D) {
        uint8_t lineR = 180, lineG = 180, lineB = 180;
        // Vertical lines
        for (int col = 0; col <= cols; ++col) {
            int x = std::min(col * beadPx, imgW - 1);
            for (int y = 0; y < imgH; ++y) {
                setPixel(img.data(), imgW, x, y, lineR, lineG, lineB);
            }
        }
        // Horizontal lines
        for (int row = 0; row <= rows; ++row) {
            int y = std::min(row * beadPx, imgH - 1);
            for (int x = 0; x < imgW; ++x) {
                setPixel(img.data(), imgW, x, y, lineR, lineG, lineB);
            }
        }
    }

    // ── Write PNG ─────────────────────────────────────────────────────────────
    int ok = stbi_write_png(path.c_str(), imgW, imgH, 4, img.data(), imgW * 4);
    if (!ok) {
        result.message = "Failed to write PNG: " + path;
        LOG_ERROR("Exporter", result.message);
        return result;
    }

    result.success = true;
    result.message = "Exported PNG (" + std::to_string(imgW) + "x" +
                     std::to_string(imgH) + ") to " + path;
    LOG_INFO("Exporter", result.message);
    return result;
}

// ── CSV export ────────────────────────────────────────────────────────────────

ExportResult Exporter::exportCsv(const std::string& path,
                                  const BeadGrid& grid,
                                  const Palette& palette) {
    ExportResult result;

    // Count beads per colour index (skip index 0 = empty)
    std::map<int, int> counts;
    int totalBeads = 0;
    int cols = grid.cols();
    int rows = grid.rows();

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            uint8_t idx = grid.get(col, row);
            if (idx == 0) continue; // skip empty
            counts[idx]++;
            totalBeads++;
        }
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        result.message = "Cannot open file for writing: " + path;
        LOG_ERROR("Exporter", result.message);
        return result;
    }

    // Header
    file << "Index,Name,R,G,B,Count\n";

    for (const auto& [idx, count] : counts) {
        ImU32 c = palette.color(idx);
        int r = static_cast<int>((c >> IM_COL32_R_SHIFT) & 0xFF);
        int g = static_cast<int>((c >> IM_COL32_G_SHIFT) & 0xFF);
        int b = static_cast<int>((c >> IM_COL32_B_SHIFT) & 0xFF);

        file << idx << ","
             << palette.name(idx) << ","
             << r << "," << g << "," << b << ","
             << count << "\n";
    }

    // Summary row (6 fields to match header: Index,Name,R,G,B,Count)
    file << ",Total,,,," << totalBeads << "\n";

    result.success = true;
    result.message = "Exported " + std::to_string(counts.size()) + " colours (" +
                     std::to_string(totalBeads) + " beads) to " + path;
    LOG_INFO("Exporter", result.message);
    return result;
}
