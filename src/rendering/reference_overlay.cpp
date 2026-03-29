#include "rendering/reference_overlay.h"
#include "logging/logger.h"

// stb_image is already compiled in image_importer.cpp — only include the
// header here so we can call stbi_load / stbi_image_free.
#include <stb_image.h>

ReferenceOverlay::~ReferenceOverlay() {
    unload();
}

// ── Load ──────────────────────────────────────────────────────────────────────

bool ReferenceOverlay::loadFromFile(const std::string& path) {
    unload();

    int w = 0, h = 0, channels = 0;
    // Force 4 channels (RGBA) so we can upload directly as GL_RGBA
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!data) {
        LOG_ERROR("ReferenceOverlay",
                  std::string("Failed to load: ") + stbi_failure_reason());
        return false;
    }

    // Upload to GPU
    glGenTextures(1, &texId_);
    glBindTexture(GL_TEXTURE_2D, texId_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    imgW_ = w;
    imgH_ = h;
    path_ = path;

    LOG_INFO("ReferenceOverlay",
             "Loaded " + path + " (" + std::to_string(w) + "x" +
             std::to_string(h) + ")");
    return true;
}

// ── Unload ────────────────────────────────────────────────────────────────────

void ReferenceOverlay::unload() {
    if (texId_) {
        glDeleteTextures(1, &texId_);
        texId_ = 0;
    }
    imgW_ = imgH_ = 0;
    path_.clear();
}
