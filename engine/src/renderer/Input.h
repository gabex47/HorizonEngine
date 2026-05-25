#pragma once

#include <glm/vec2.hpp>

struct GLFWwindow;

namespace Horizon {

enum class Key {
    W,
    A,
    S,
    D,
    Q,
    E,
    LeftShift,
    RightShift,
    Escape
};

enum class MouseButton {
    Right
};

class Input final {
public:
    void Attach(GLFWwindow* window);
    void Update();
    void EndFrame();
    bool GetKey(Key key) const;
    bool GetMouseButton(MouseButton button) const;
    glm::vec2 GetMouseDelta() const;
    float GetScroll() const;
    void SetCursorCaptured(bool captured);
    bool IsCursorCaptured() const;

private:
    static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

    GLFWwindow* window_ = nullptr;
    glm::vec2 mouseDelta_{0.0f, 0.0f};
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
    float scrollY_ = 0.0f;
    bool firstMouseSample_ = true;
    bool cursorCaptured_ = false;
};

} // namespace Horizon
