#pragma once

#include "Instance.h"

#include <string>

namespace Horizon {

class Folder final : public Instance {
public:
    std::string GetClass() override { return "Folder"; }
};

} // namespace Horizon
