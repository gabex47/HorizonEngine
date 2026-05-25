#pragma once

#include "../Instance.h"

#include <functional>
#include <string>
#include <vector>

namespace Horizon {

class HorizonFunction final : public Instance {
public:
    using Handler = std::function<std::string(const std::vector<std::string>& args)>;

    std::string InvokeServer(std::vector<std::string> args);
    void OnServerInvoke(Handler handler);
    std::string GetClass() override;

private:
    Handler serverHandler;
};

} // namespace Horizon
