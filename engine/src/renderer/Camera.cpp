#include "Camera.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

namespace Horizon {

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(
        glm::vec3(Position.X, Position.Y, Position.Z),
        glm::vec3(Target.X, Target.Y, Target.Z),
        glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(FOV), aspectRatio, 0.1f, 100.0f);
}

} // namespace Horizon
