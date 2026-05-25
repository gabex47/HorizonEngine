#pragma once

#include "../Instance.h"

#include <functional>
#include <string>
#include <vector>

namespace Horizon {

class HorizonEvent final : public Instance {
public:
    using Callback = std::function<void(const std::vector<std::string>& args)>;

    void FireServer(std::vector<std::string> args);
    void FireClient(std::vector<std::string> args);
    void FireAllClients(std::vector<std::string> args);
    void OnServerEvent(Callback cb);
    void OnClientEvent(Callback cb);
    std::string GetClass() override;

private:
    Callback serverCallback;
    Callback clientCallback;
};

} // namespace Horizon
