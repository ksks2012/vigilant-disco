#ifndef REFERENCE_OVERLAY_H
#define REFERENCE_OVERLAY_H

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include <string>

// Manages a GPU texture loaded from an image file for use as a
// semi-transparent reference layer on the canvas.
// The texture is uploaded once and reused each frame.
class ReferenceOverlay {
public:
    ReferenceOverlay() = default;
    ~ReferenceOverlay();

    // Non-copyable
    ReferenceOverlay(const ReferenceOverlay&) = delete;
    ReferenceOverlay& operator=(const ReferenceOverlay&) = delete;

    // Load an image file (PNG, JPG, BMP, etc.) and upload to GPU.
    // Returns true on success.  Any previous texture is released first.
    bool loadFromFile(const std::string& path);

    // Release the GPU texture.
    void unload();

    // True if a texture is currently loaded.
    bool isLoaded() const { return texId_ != 0; }

    // The OpenGL texture ID (cast to ImTextureID for ImGui::Image).
    GLuint textureId() const { return texId_; }

    // Original image dimensions in pixels.
    int imageWidth()  const { return imgW_; }
    int imageHeight() const { return imgH_; }

    // The path that was last loaded (empty if none).
    const std::string& path() const { return path_; }

private:
    GLuint      texId_ = 0;
    int         imgW_  = 0;
    int         imgH_  = 0;
    std::string path_;
};

#endif // REFERENCE_OVERLAY_H
