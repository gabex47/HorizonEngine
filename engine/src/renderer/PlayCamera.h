#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

struct GLFWwindow;

namespace Horizon {

class PlayCamera final {
public:
    float orbitYaw = 0.0f;
    float orbitPitch = 20.0f;
    float orbitDist = 15.0f;

    glm::mat4 GetViewMatrix(glm::vec3 playerPos);
    glm::vec3 GetForwardDir();
    void Update(GLFWwindow* window, float deltaTime, float mouseDeltaX, float mouseDeltaY, bool leftMouseHeld);
};

} // namespace Horizon
