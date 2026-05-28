#pragma once

#include "Script.h"

namespace Horizon {

class ServerScript final : public Script {
public:
    std::string GetClass() override { return "ServerScript"; }
};

} // namespace Horizon
