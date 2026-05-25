#include "Renderer.h"

#include "services/Scene.h"

#define GLFW_INCLUDE_NONE
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

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
} // namespace
namespace Horizon {

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

void Renderer::RenderPart(Part& part)
{
    int width = 1, height = 1;
    glfwGetFramebufferSize(window, &width, &height);
    const float aspect = static_cast<float>(width) / static_cast<float>(height == 0 ? 1 : height);

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(part.Position.X, part.Position.Y, part.Position.Z));
    model = glm::scale(model, glm::vec3(part.Size.X, part.Size.Y, part.Size.Z));

    const glm::mat4 mvp = camera.GetProjectionMatrix(aspect) * camera.GetViewMatrix() * model;
    const glm::vec3 color(part.Color.R / 255.0f, part.Color.G / 255.0f, part.Color.B / 255.0f);

    shader.Use();
    shader.SetMat4("uMVP", mvp);
    shader.SetVec3("uColor", color);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Renderer::RenderScene()
{
    int width = 1, height = 1;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.06f, 0.06f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (const auto& child : Scene::Get().GetChildren())
        if (auto part = std::dynamic_pointer_cast<Part>(child))
            RenderPart(*part);

    glfwSwapBuffers(window);
}

void Renderer::Shutdown()
{
    shader.Destroy();
    if (vbo != 0)
        glDeleteBuffers(1, &vbo);
    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
    if (window)
        glfwDestroyWindow(window);

    vbo = 0;
    vao = 0;
    window = nullptr;
    glfwTerminate();
}

} // namespace Horizon
