#include "LoopService.h"

#include <utility>
#include <vector>

namespace Horizon {

LoopService& LoopService::Get()
{
    static LoopService service;
    return service;
}

void LoopService::BindToHeartbeat(const std::string& name, Callback cb)
{
    heartbeatBindings[name] = std::move(cb);
}

void LoopService::BindToStepped(const std::string& name, Callback cb)
{
    steppedBindings[name] = std::move(cb);
}

void LoopService::UnbindFromHeartbeat(const std::string& name)
{
    heartbeatBindings.erase(name);
}

void LoopService::UnbindFromStepped(const std::string& name)
{
    steppedBindings.erase(name);
}

void LoopService::Tick(float deltaTime)
{
    const auto heartbeat = std::vector(heartbeatBindings.begin(), heartbeatBindings.end());
    for (const auto& entry : heartbeat)
        if (entry.second)
            entry.second(deltaTime);

    const auto stepped = std::vector(steppedBindings.begin(), steppedBindings.end());
    for (const auto& entry : stepped)
        if (entry.second)
            entry.second(deltaTime);
}

} // namespace Horizon
