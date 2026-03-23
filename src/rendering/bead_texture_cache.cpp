#include "rendering/bead_texture_cache.h"

#include <cstring>
#include <cmath>
#include <algorithm>

// ── Helpers ───────────────────────────────────────────────────────────────────

// Unpack ImU32 (ABGR) to separate R, G, B components
static void unpackColor(ImU32 col, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = static_cast<uint8_t>((col >>  0) & 0xFF); // IM_COL32_R_SHIFT = 0
    g = static_cast<uint8_t>((col >>  8) & 0xFF);
    b = static_cast<uint8_t>((col >> 16) & 0xFF);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

BeadTextureCache::BeadTextureCache() = default;

BeadTextureCache::~BeadTextureCache() {
    destroyFBO();
}

void BeadTextureCache::markDirty() {
    dirty_ = true;
}

// ── FBO management ────────────────────────────────────────────────────────────

void BeadTextureCache::destroyFBO() {
    if (fboId_) { glDeleteFramebuffers(1, &fboId_); fboId_ = 0; }
    if (texId_) { glDeleteTextures(1, &texId_);     texId_ = 0; }
    texW_ = texH_ = 0;
}

void BeadTextureCache::ensureFBO(int w, int h) {
    if (texId_ && texW_ == w && texH_ == h) return;

    destroyFBO();

    // Create texture
    glGenTextures(1, &texId_);
    glBindTexture(GL_TEXTURE_2D, texId_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create FBO
    glGenFramebuffers(1, &fboId_);
    glBindFramebuffer(GL_FRAMEBUFFER, fboId_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, texId_, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    texW_ = w;
    texH_ = h;
}

// ── Draw a single bead into a CPU pixel buffer ───────────────────────────────

void BeadTextureCache::drawBead(std::vector<uint8_t>& pixels,
                                 int px, int py, int beadPx,
                                 uint8_t r, uint8_t g, uint8_t b,
                                 bool isEmpty) const {
    // Bead fills most of the cell as a circle
    float centre  = static_cast<float>(beadPx) * 0.5f;
    float radius  = centre * 0.88f;   // kBeadRadius / 0.5 ≈ 0.80 → 0.88 scale
    float radiusSq = radius * radius;

    // Outline ring
    float outR    = centre * 0.96f;   // kOutlineRadius / 0.5 ≈ 0.88 → 0.96 scale
    float outRSq  = outR * outR;

    // Board colour (matches kBoardColor)
    constexpr uint8_t boardR = 230, boardG = 230, boardB = 235;
    // Outline colour (matches kOutlineColor)
    constexpr uint8_t outlineR = 200, outlineG = 200, outlineB = 200;
    // Peg colour (matches kPegColor)
    constexpr uint8_t pegR = 180, pegG = 180, pegB = 185;

    float pegRadius = centre * 0.15f;
    float pegRadiusSq = pegRadius * pegRadius;

    int stride = texW_ * 4;

    for (int dy = 0; dy < beadPx; ++dy) {
        int imgY = py + dy;
        if (imgY < 0 || imgY >= texH_) continue;

        for (int dx = 0; dx < beadPx; ++dx) {
            int imgX = px + dx;
            if (imgX < 0 || imgX >= texW_) continue;

            float fx = static_cast<float>(dx) + 0.5f - centre;
            float fy = static_cast<float>(dy) + 0.5f - centre;
            float distSq = fx * fx + fy * fy;

            int offset = imgY * stride + imgX * 4;

            if (distSq <= radiusSq) {
                // Inside bead
                if (isEmpty && distSq <= pegRadiusSq) {
                    // Centre peg for empty beads
                    pixels[offset + 0] = pegR;
                    pixels[offset + 1] = pegG;
                    pixels[offset + 2] = pegB;
                    pixels[offset + 3] = 255;
                } else {
                    pixels[offset + 0] = r;
                    pixels[offset + 1] = g;
                    pixels[offset + 2] = b;
                    pixels[offset + 3] = 255;
                }
            } else if (distSq <= outRSq) {
                // Outline ring
                pixels[offset + 0] = outlineR;
                pixels[offset + 1] = outlineG;
                pixels[offset + 2] = outlineB;
                pixels[offset + 3] = 255;
            } else {
                // Board background
                pixels[offset + 0] = boardR;
                pixels[offset + 1] = boardG;
                pixels[offset + 2] = boardB;
                pixels[offset + 3] = 255;
            }
        }
    }
}

// ── Main update ───────────────────────────────────────────────────────────────

void BeadTextureCache::update(const BeadGrid& grid, const Palette& palette,
                               int beadPx) {
    if (!dirty_) return;

    int cols = grid.cols();
    int rows = grid.rows();
    int w = cols * beadPx;
    int h = rows * beadPx;

    if (w <= 0 || h <= 0) return;

    // Clamp texture size to reasonable limits (e.g. 4096x4096)
    constexpr int kMaxTexSize = 4096;
    if (w > kMaxTexSize || h > kMaxTexSize) {
        float scaleDown = static_cast<float>(kMaxTexSize) /
                          static_cast<float>(std::max(w, h));
        beadPx = std::max(2, static_cast<int>(beadPx * scaleDown));
        w = cols * beadPx;
        h = rows * beadPx;
    }

    gridCols_ = cols;
    gridRows_ = rows;

    ensureFBO(w, h);

    // Render into CPU buffer
    std::vector<uint8_t> pixels(w * h * 4);

    // Fill with board colour first
    for (int i = 0; i < w * h; ++i) {
        pixels[i * 4 + 0] = 230;
        pixels[i * 4 + 1] = 230;
        pixels[i * 4 + 2] = 235;
        pixels[i * 4 + 3] = 255;
    }

    // Draw each bead
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            uint8_t colorIdx = grid.get(col, row);
            ImU32 color = palette.color(colorIdx);

            uint8_t r, g, b;
            unpackColor(color, r, g, b);

            int px = col * beadPx;
            int py = row * beadPx;
            drawBead(pixels, px, py, beadPx, r, g, b, colorIdx == 0);
        }
    }

    // Upload to GPU
    glBindTexture(GL_TEXTURE_2D, texId_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    dirty_ = false;
}
