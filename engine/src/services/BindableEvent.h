#pragma once

#include "../Instance.h"

#include <functional>
#include <string>
#include <vector>

namespace Horizon {

class BindableEvent : public Instance {
public:
    using Callback = std::function<void(const std::vector<std::string>& args)>;

    void Fire(std::vector<std::string> args);
    void Connect(Callback cb);
    void DisconnectAll();
    std::string GetClass() override { return "BindableEvent"; }

private:
    std::vector<Callback> callbacks;
};

} // namespace Horizon
