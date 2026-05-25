#pragma once

#include "Horizon/Types.h"
#include "Instance.h"

namespace Horizon {

class Part final : public Instance {
public:
    Vector3 Position;
    Vector3 Size;
    Color3 Color;

    Part();
    std::string GetClass() override;
};

} // namespace Horizon
