#pragma once

#include "Script.h"

namespace Horizon {

class LocalScript final : public Script {
public:
    std::string GetClass() override { return "LocalScript"; }
};

} // namespace Horizon
