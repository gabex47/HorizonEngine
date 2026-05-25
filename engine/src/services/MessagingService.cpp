#include "MessagingService.h"

#include <utility>

namespace Horizon {

MessagingService& MessagingService::Get()
{
    static MessagingService service;
    return service;
}

void MessagingService::PublishAsync(const std::string& topic, const std::string& message)
{
    const auto found = subscribers.find(topic);
    if (found == subscribers.end())
        return;

    const auto callbacks = found->second;
    for (const auto& callback : callbacks)
        if (callback)
            callback(message);
}

void MessagingService::SubscribeAsync(const std::string& topic, Callback cb)
{
    subscribers[topic].push_back(std::move(cb));
}

void MessagingService::Unsubscribe(const std::string& topic)
{
    subscribers.erase(topic);
}

} // namespace Horizon
