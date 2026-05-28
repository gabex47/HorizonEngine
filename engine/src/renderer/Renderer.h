#pragma once

#include "Camera.h"
#include "EditorCamera.h"
#include "Input.h"
#include "Part.h"
#include "PlayCamera.h"
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
    void InstallInputCallbacks();
    void Update(float deltaTime, bool cameraInputEnabled);
    void RenderPart(Part& part);
    void RenderGround();
    void RenderScene();
    void ClearDefaultFramebuffer();
    void SwapBuffers();
    void SetCursorLocked(bool locked);
    bool GetKey(Key key) const;
    void SetPlayCameraTarget(const glm::vec3& target);
    glm::vec3 GetPlayCameraForwardDir();
    float GetPlayCameraDistance() const;
    void ResetPlayCamera();
    GLFWwindow* GetWindow() const;
    void Shutdown();

private:
    void UpdateSceneViewport();
    void RenderInstanceRecursive(std::shared_ptr<Instance> instance);
    void RenderPart(Part& part, const glm::mat4& view, const glm::mat4& projection);
    void RenderGround(const glm::mat4& view, const glm::mat4& projection);
    glm::mat4 GetSceneViewMatrix();
    glm::mat4 GetSceneProjectionMatrix() const;

    GLFWwindow* window = nullptr;
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int groundVao = 0;
    unsigned int groundVbo = 0;
    unsigned int gridVao = 0;
    unsigned int gridVbo = 0;
    int gridVertexCount = 0;
    Viewport sceneViewport;
    SceneCameraMode sceneCameraMode = SceneCameraMode::Editor;
    EditorCamera editorCamera;
    Camera gameCamera;
    PlayCamera playCamera;
    glm::vec3 playCameraTarget{0.0f, 1.0f, 0.0f};
    glm::mat4 activeView{1.0f};
    glm::mat4 activeProjection{1.0f};
    bool transparentRenderPass = false;
    Input input;
    Shader shader;
};

} // namespace Horizon
