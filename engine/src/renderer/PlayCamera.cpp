#include "PlayCamera.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

namespace Horizon {

glm::mat4 PlayCamera::GetViewMatrix(glm::vec3 playerPos)
{
    const float radYaw = glm::radians(orbitYaw);
    const float radPitch = glm::radians(orbitPitch);
    const glm::vec3 offset{
        orbitDist * std::cos(radPitch) * std::sin(radYaw),
        orbitDist * std::sin(radPitch),
        orbitDist * std::cos(radPitch) * std::cos(radYaw)};
    const glm::vec3 camPos = playerPos + offset;
    const glm::vec3 target = playerPos + glm::vec3(0.0f, 1.0f, 0.0f);
    return glm::lookAt(camPos, target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 PlayCamera::GetForwardDir()
{
    const float rad = glm::radians(orbitYaw);
    return glm::normalize(glm::vec3(-std::sin(rad), 0.0f, -std::cos(rad)));
}

void PlayCamera::Update(GLFWwindow* window, float deltaTime, float mouseDeltaX, float mouseDeltaY, bool leftMouseHeld)
{
    if (!window)
        return;

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        orbitDist -= 30.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        orbitDist += 30.0f * deltaTime;
    orbitDist = std::clamp(orbitDist, 2.0f, 50.0f);

    const bool firstPerson = orbitDist <= 2.0f;
    if (leftMouseHeld && !firstPerson)
    {
        orbitYaw += mouseDeltaX * 0.3f;
        orbitPitch -= mouseDeltaY * 0.3f;
        orbitPitch = std::clamp(orbitPitch, -80.0f, 80.0f);
    }

    if (firstPerson)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        orbitYaw += mouseDeltaX * 0.3f;
        orbitPitch -= mouseDeltaY * 0.3f;
        orbitPitch = std::clamp(orbitPitch, -80.0f, 80.0f);
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

} // namespace Horizon
