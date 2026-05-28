#pragma once

#include "Script.h"

namespace Horizon {

class ModuleScript final : public Script {
public:
    std::string GetClass() override { return "ModuleScript"; }
};

} // namespace Horizon
