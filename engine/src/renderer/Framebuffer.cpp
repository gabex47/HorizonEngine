#include "Framebuffer.h"

#include <algorithm>
#include <iostream>

namespace Horizon {

Framebuffer::Framebuffer(int initialWidth, int initialHeight)
    : width(std::max(initialWidth, 1))
    , height(std::max(initialHeight, 1))
{
    Create();
}

Framebuffer::~Framebuffer()
{
    Destroy();
}

void Framebuffer::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
}

void Framebuffer::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Resize(int w, int h)
{
    w = std::max(w, 1);
    h = std::max(h, 1);
    if (w == width && h == height)
        return;

    GLint previousFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    const bool wasBound = previousFramebuffer == static_cast<GLint>(fbo);
    if (wasBound)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Destroy();
    width = w;
    height = h;
    Create();

    if (wasBound)
        Bind();
    else
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
}

GLuint Framebuffer::GetTexture()
{
    return colorTexture;
}

int Framebuffer::GetWidth()
{
    return width;
}

int Framebuffer::GetHeight()
{
    return height;
}

void Framebuffer::Create()
{
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[ERROR] Framebuffer incomplete" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Destroy()
{
    GLint previousFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    if (previousFramebuffer == static_cast<GLint>(fbo))
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (rbo != 0)
        glDeleteRenderbuffers(1, &rbo);
    if (colorTexture != 0)
        glDeleteTextures(1, &colorTexture);
    if (fbo != 0)
        glDeleteFramebuffers(1, &fbo);

    rbo = 0;
    colorTexture = 0;
    fbo = 0;
}

} // namespace Horizon
