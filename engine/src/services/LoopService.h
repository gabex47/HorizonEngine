#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace Horizon {

class LoopService final {
public:
    using Callback = std::function<void(float deltaTime)>;

    void BindToHeartbeat(const std::string& name, Callback cb);
    void BindToStepped(const std::string& name, Callback cb);
    void UnbindFromHeartbeat(const std::string& name);
    void UnbindFromStepped(const std::string& name);
    void Tick(float deltaTime);
    static LoopService& Get();

private:
    LoopService() = default;

    std::unordered_map<std::string, Callback> heartbeatBindings;
    std::unordered_map<std::string, Callback> steppedBindings;
};

} // namespace Horizon
