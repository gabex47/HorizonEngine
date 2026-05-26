#pragma once

#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#include <OpenGL/gl3.h>

namespace Horizon {

class Framebuffer final {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void Bind();
    void Unbind();
    void Resize(int w, int h);
    GLuint GetTexture();
    int GetWidth();
    int GetHeight();

private:
    void Create();
    void Destroy();

    GLuint fbo = 0;
    GLuint colorTexture = 0;
    GLuint rbo = 0;
    int width = 1;
    int height = 1;
};

} // namespace Horizon
