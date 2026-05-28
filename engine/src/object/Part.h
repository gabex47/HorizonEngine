#pragma once

#include "Horizon/Types.h"
#include "Instance.h"

#include <string>

namespace Horizon {

class Part final : public Instance {
public:
    Vector3 Position;
    Vector3 Size;
    Color3 Color;
    Vector3 Rotation = {0.0f, 0.0f, 0.0f};
    float Transparency = 0.0f;
    bool Anchored = true;
    bool CanCollide = true;
    std::string Material = "SmoothPlastic";

    Part();
    std::string GetClass() override;
};

} // namespace Horizon
