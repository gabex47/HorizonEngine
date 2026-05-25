#include "BindableEvent.h"

#include <utility>

namespace Horizon {

void BindableEvent::Fire(std::vector<std::string> args)
{
    const auto snapshot = callbacks;
    for (const auto& callback : snapshot)
        if (callback)
            callback(args);
}

void BindableEvent::Connect(Callback cb)
{
    callbacks.push_back(std::move(cb));
}

void BindableEvent::DisconnectAll()
{
    callbacks.clear();
}

} // namespace Horizon
