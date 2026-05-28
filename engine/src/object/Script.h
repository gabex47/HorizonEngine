#pragma once

#include "Instance.h"

#include <string>

namespace Horizon {

class Script : public Instance {
public:
    std::string Source = "-- Script";
    bool Enabled = true;
    std::string GetClass() override { return "Script"; }
};

} // namespace Horizon
