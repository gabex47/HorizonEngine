#pragma once

#include "Camera.h"
#include "Part.h"
#include "Shader.h"

struct GLFWwindow;

namespace Horizon {

class Renderer final {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool Init(int width, int height, const char* title);
    bool ShouldClose() const;
    void PollEvents() const;
    void RenderPart(Part& part);
    void RenderScene();
    void Shutdown();

private:
    GLFWwindow* window = nullptr;
    unsigned int vao = 0;
    unsigned int vbo = 0;
    Camera camera;
    Shader shader;
};

} // namespace Horizon
