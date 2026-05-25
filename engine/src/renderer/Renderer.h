#pragma once

#include "Camera.h"
#include "EditorCamera.h"
#include "Input.h"
#include "Part.h"
#include "Shader.h"

#include <glm/mat4x4.hpp>

struct GLFWwindow;

namespace Horizon {

struct Viewport {
    int X = 0;
    int Y = 0;
    int Width = 1;
    int Height = 1;

    float AspectRatio() const;
};

enum class SceneCameraMode {
    Editor,
    Game
};

class Renderer final {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool Init(int width, int height, const char* title);
    bool ShouldClose() const;
    void PollEvents() const;
    void Update(float deltaTime);
    void RenderPart(Part& part);
    void RenderScene();
    void Shutdown();

private:
    void UpdateSceneViewport();
    void RenderPart(Part& part, const glm::mat4& view, const glm::mat4& projection);
    glm::mat4 GetSceneViewMatrix() const;
    glm::mat4 GetSceneProjectionMatrix() const;

    GLFWwindow* window = nullptr;
    unsigned int vao = 0;
    unsigned int vbo = 0;
    Viewport sceneViewport;
    SceneCameraMode sceneCameraMode = SceneCameraMode::Editor;
    EditorCamera editorCamera;
    Camera gameCamera;
    Input input;
    Shader shader;
};

} // namespace Horizon
