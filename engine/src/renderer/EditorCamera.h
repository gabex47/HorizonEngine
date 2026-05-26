#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace Horizon {

class Input;

class EditorCamera final {
public:
    EditorCamera();

    void Update(float deltaTime, Input& input, bool lookEnabled);
    void HandleKeyboard(float deltaTime, const Input& input);
    void HandleMouse(Input& input);
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;

    void FocusSelected();
    void OrbitMode();
    void Bookmarks();
    void FlyMode();

private:
    void UpdateVectors();

    glm::vec3 position_{0.0f, 5.0f, 10.0f};
    glm::vec3 front_{0.0f, 0.0f, -1.0f};
    glm::vec3 right_{1.0f, 0.0f, 0.0f};
    glm::vec3 up_{0.0f, 1.0f, 0.0f};
    glm::vec3 worldUp_{0.0f, 1.0f, 0.0f};
    float yaw_ = -90.0f;
    float pitch_ = -20.0f;
    float speed_ = 8.0f;
    float sensitivity_ = 0.1f;
    float fov_ = 60.0f;
};

} // namespace Horizon
