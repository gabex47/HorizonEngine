#pragma once

#include "Horizon/Types.h"
#include "Instance.h"

#include <string>

namespace Horizon {

class SpotLight final : public Instance {
public:
    Color3 Color = {255, 255, 200};
    float Brightness = 1.0f;
    float Range = 20.0f;
    float Angle = 45.0f;
    bool Enabled = true;
    std::string GetClass() override { return "SpotLight"; }
};

} // namespace Horizon
