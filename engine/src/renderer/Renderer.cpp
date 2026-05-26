#include "Renderer.h"

#include "services/Scene.h"

#define GLFW_INCLUDE_NONE
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <vector>

namespace {
constexpr const char* kVertexShader = R"(#version 330 core
layout (location = 0) in vec3 aPosition;
uniform mat4 uMVP;
void main()
{
    gl_Position = uMVP * vec4(aPosition, 1.0);
}
)";
constexpr const char* kFragmentShader = R"(#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main()
{
    FragColor = vec4(uColor, 1.0);
}
)";

constexpr float kCubeVertices[] = {
    -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f,
    0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
    -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f,
    0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f,
};

constexpr float kGroundVertices[] = {
    -25.0f, 0.0f, -25.0f, 25.0f, 0.0f, -25.0f, 25.0f, 0.0f, 25.0f,
    25.0f, 0.0f, 25.0f, -25.0f, 0.0f, 25.0f, -25.0f, 0.0f, -25.0f,
};

std::vector<float> buildGridVertices()
{
    std::vector<float> vertices;
    vertices.reserve(51 * 12);
    for (int i = -25; i <= 25; ++i)
    {
        vertices.insert(vertices.end(), {static_cast<float>(i), 0.01f, -25.0f, static_cast<float>(i), 0.01f, 25.0f});
        vertices.insert(vertices.end(), {-25.0f, 0.01f, static_cast<float>(i), 25.0f, 0.01f, static_cast<float>(i)});
    }
    return vertices;
}
} // namespace
namespace Horizon {

float Viewport::AspectRatio() const
{
    return static_cast<float>(Width) / static_cast<float>(Height == 0 ? 1 : Height);
}

Renderer::~Renderer()
{
    Shutdown();
}

bool Renderer::Init(int width, int height, const char* title)
{
    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window)
        return false;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    input.Attach(window);

    glEnable(GL_DEPTH_TEST);
    if (!shader.Load(kVertexShader, kFragmentShader))
        return false;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &groundVao);
    glGenBuffers(1, &groundVbo);
    glBindVertexArray(groundVao);
    glBindBuffer(GL_ARRAY_BUFFER, groundVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kGroundVertices), kGroundVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    const auto gridVertices = buildGridVertices();
    gridVertexCount = static_cast<int>(gridVertices.size() / 3);
    glGenVertexArrays(1, &gridVao);
    glGenBuffers(1, &gridVbo);
    glBindVertexArray(gridVao);
    glBindBuffer(GL_ARRAY_BUFFER, gridVbo);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    return true;
}

bool Renderer::ShouldClose() const
{
    return window && glfwWindowShouldClose(window);
}

void Renderer::PollEvents() const
{
    glfwPollEvents();
}

void Renderer::InstallInputCallbacks()
{
    input.Attach(window);
}

void Renderer::Update(float deltaTime, bool cameraInputEnabled)
{
    input.Update();
    editorCamera.Update(deltaTime, input, cameraInputEnabled);
    input.EndFrame();
}

void Renderer::RenderPart(Part& part)
{
    UpdateSceneViewport();
    RenderPart(part, GetSceneViewMatrix(), GetSceneProjectionMatrix());
}

void Renderer::RenderPart(Part& part, const glm::mat4& view, const glm::mat4& projection)
{
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(part.Position.X, part.Position.Y, part.Position.Z));
    model = glm::scale(model, glm::vec3(part.Size.X, part.Size.Y, part.Size.Z));

    const glm::mat4 mvp = projection * view * model;
    const glm::vec3 color(part.Color.R / 255.0f, part.Color.G / 255.0f, part.Color.B / 255.0f);

    shader.Use();
    shader.SetMat4("uMVP", mvp);
    shader.SetVec3("uColor", color);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Renderer::RenderGround()
{
    UpdateSceneViewport();
    RenderGround(GetSceneViewMatrix(), GetSceneProjectionMatrix());
}

void Renderer::RenderGround(const glm::mat4& view, const glm::mat4& projection)
{
    static bool logged = false;
    if (!logged) {
        std::cout << "[DEBUG] RenderGround called" << std::endl;
        logged = true;
    }

    const glm::mat4 mvp = projection * view * glm::mat4(1.0f);
    shader.Use();
    shader.SetMat4("uMVP", mvp);
    shader.SetVec3("uColor", glm::vec3(0.15f, 0.15f, 0.15f));
    glBindVertexArray(groundVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    shader.SetVec3("uColor", glm::vec3(0.25f, 0.25f, 0.25f));
    glBindVertexArray(gridVao);
    glDrawArrays(GL_LINES, 0, gridVertexCount);
}

void Renderer::RenderScene()
{
    UpdateSceneViewport();
    const glm::mat4 view = GetSceneViewMatrix();
    const glm::mat4 projection = GetSceneProjectionMatrix();

    glViewport(sceneViewport.X, sceneViewport.Y, sceneViewport.Width, sceneViewport.Height);
    glClearColor(0.06f, 0.06f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderGround(view, projection);
    for (const auto& child : Scene::Get().GetChildren())
        if (auto part = std::dynamic_pointer_cast<Part>(child))
            RenderPart(*part, view, projection);
}

void Renderer::UpdateSceneViewport()
{
    GLint viewport[4] = {0, 0, 1, 1};
    glGetIntegerv(GL_VIEWPORT, viewport);
    sceneViewport.X = viewport[0];
    sceneViewport.Y = viewport[1];
    sceneViewport.Width = viewport[2];
    sceneViewport.Height = viewport[3];
}

glm::mat4 Renderer::GetSceneViewMatrix() const
{
    if (sceneCameraMode == SceneCameraMode::Game)
        return gameCamera.GetViewMatrix();
    return editorCamera.GetViewMatrix();
}

glm::mat4 Renderer::GetSceneProjectionMatrix() const
{
    if (sceneCameraMode == SceneCameraMode::Game)
        return gameCamera.GetProjectionMatrix(sceneViewport.AspectRatio());
    return editorCamera.GetProjectionMatrix(sceneViewport.AspectRatio());
}

void Renderer::ClearDefaultFramebuffer()
{
    int width = 1, height = 1;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.10f, 0.10f, 0.13f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SwapBuffers()
{
    glfwSwapBuffers(window);
}

void Renderer::SetCursorLocked(bool locked)
{
    input.SetCursorCaptured(locked);
}

bool Renderer::GetKey(Key key) const
{
    return input.GetKey(key);
}

GLFWwindow* Renderer::GetWindow() const
{
    return window;
}

void Renderer::Shutdown()
{
    shader.Destroy();
    if (gridVbo != 0)
        glDeleteBuffers(1, &gridVbo);
    if (gridVao != 0)
        glDeleteVertexArrays(1, &gridVao);
    if (groundVbo != 0)
        glDeleteBuffers(1, &groundVbo);
    if (groundVao != 0)
        glDeleteVertexArrays(1, &groundVao);
    if (vbo != 0)
        glDeleteBuffers(1, &vbo);
    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
    if (window)
        glfwDestroyWindow(window);

    vbo = 0;
    vao = 0;
    groundVbo = 0;
    groundVao = 0;
    gridVbo = 0;
    gridVao = 0;
    window = nullptr;
    glfwTerminate();
}

} // namespace Horizon
