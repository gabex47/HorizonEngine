#pragma once

#include "Horizon/Types.h"

#include <glm/mat4x4.hpp>

namespace Horizon {

class Camera final {
public:
    Vector3 Position{0.0f, 2.0f, 6.0f};
    Vector3 Target{0.0f, 0.0f, 0.0f};
    float FOV = 60.0f;

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;
};

} // namespace Horizon
