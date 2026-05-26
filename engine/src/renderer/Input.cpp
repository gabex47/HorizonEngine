#include "Input.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace {

int toGlfwKey(Horizon::Key key)
{
    switch (key)
    {
    case Horizon::Key::W: return GLFW_KEY_W;
    case Horizon::Key::A: return GLFW_KEY_A;
    case Horizon::Key::S: return GLFW_KEY_S;
    case Horizon::Key::D: return GLFW_KEY_D;
    case Horizon::Key::Q: return GLFW_KEY_Q;
    case Horizon::Key::E: return GLFW_KEY_E;
    case Horizon::Key::Tab: return GLFW_KEY_TAB;
    case Horizon::Key::LeftShift: return GLFW_KEY_LEFT_SHIFT;
    case Horizon::Key::RightShift: return GLFW_KEY_RIGHT_SHIFT;
    case Horizon::Key::Escape: return GLFW_KEY_ESCAPE;
    }
    return GLFW_KEY_UNKNOWN;
}

int toGlfwMouseButton(Horizon::MouseButton button)
{
    return button == Horizon::MouseButton::Right ? GLFW_MOUSE_BUTTON_RIGHT : GLFW_MOUSE_BUTTON_LEFT;
}

} // namespace

namespace Horizon {

void Input::Attach(GLFWwindow* window)
{
    window_ = window;
    glfwSetWindowUserPointer(window_, this);
    glfwSetScrollCallback(window_, ScrollCallback);
    firstMouseSample_ = true;
}

void Input::Update()
{
    mouseDelta_ = {0.0f, 0.0f};
    if (!window_)
        return;

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window_, &x, &y);
    if (!firstMouseSample_)
        mouseDelta_ = {static_cast<float>(x - lastMouseX_), static_cast<float>(lastMouseY_ - y)};

    lastMouseX_ = x;
    lastMouseY_ = y;
    firstMouseSample_ = false;
}

void Input::EndFrame()
{
    scrollY_ = 0.0f;
}

bool Input::GetKey(Key key) const
{
    return window_ && glfwGetKey(window_, toGlfwKey(key)) == GLFW_PRESS;
}

bool Input::GetMouseButton(MouseButton button) const
{
    return window_ && glfwGetMouseButton(window_, toGlfwMouseButton(button)) == GLFW_PRESS;
}

glm::vec2 Input::GetMouseDelta() const
{
    return mouseDelta_;
}

float Input::GetScroll() const
{
    return scrollY_;
}

void Input::SetCursorCaptured(bool captured)
{
    if (!window_ || cursorCaptured_ == captured)
        return;

    cursorCaptured_ = captured;
    firstMouseSample_ = true;
    mouseDelta_ = {0.0f, 0.0f};
    glfwSetInputMode(window_, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

bool Input::IsCursorCaptured() const
{
    return cursorCaptured_;
}

void Input::ScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    if (ImGui::GetCurrentContext())
        ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);

    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (input)
        input->scrollY_ += static_cast<float>(yOffset);
}

} // namespace Horizon
