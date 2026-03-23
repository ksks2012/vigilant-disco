#ifndef BEAD_TEXTURE_CACHE_H
#define BEAD_TEXTURE_CACHE_H

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "core/bead_grid.h"
#include "core/palette.h"

// Renders the bead grid into an off-screen OpenGL texture (FBO).
// The texture is only regenerated when markDirty() is called.
// This avoids rebuilding thousands of ImGui DrawList vertices every frame.
class BeadTextureCache {
public:
    BeadTextureCache();
    ~BeadTextureCache();

    // Non-copyable
    BeadTextureCache(const BeadTextureCache&) = delete;
    BeadTextureCache& operator=(const BeadTextureCache&) = delete;

    // Mark the cache as dirty (must be called whenever grid data changes)
    void markDirty();

    // Rebuild the texture if dirty. Must be called while an OpenGL context
    // is current. 'beadPx' is the pixel size per bead in the texture.
    void update(const BeadGrid& grid, const Palette& palette, int beadPx = 16);

    // The OpenGL texture ID (cast to ImTextureID for ImGui::Image)
    GLuint textureId() const { return texId_; }

    // Texture dimensions in pixels
    int texWidth()  const { return texW_; }
    int texHeight() const { return texH_; }

    // Grid dimensions the texture was built for
    int gridCols() const { return gridCols_; }
    int gridRows() const { return gridRows_; }

    bool isDirty() const { return dirty_; }

private:
    void ensureFBO(int w, int h);
    void destroyFBO();

    // Draw one bead into the pixel buffer
    void drawBead(std::vector<uint8_t>& pixels, int px, int py, int beadPx,
                  uint8_t r, uint8_t g, uint8_t b, bool isEmpty) const;

    GLuint fboId_  = 0;
    GLuint texId_  = 0;
    int    texW_   = 0;
    int    texH_   = 0;
    int    gridCols_ = 0;
    int    gridRows_ = 0;
    bool   dirty_  = true;
};

#endif // BEAD_TEXTURE_CACHE_H
