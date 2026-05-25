#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Horizon {

class MessagingService final {
public:
    using Callback = std::function<void(const std::string& message)>;

    void PublishAsync(const std::string& topic, const std::string& message);
    void SubscribeAsync(const std::string& topic, Callback cb);
    void Unsubscribe(const std::string& topic);
    static MessagingService& Get();

private:
    MessagingService() = default;

    std::unordered_map<std::string, std::vector<Callback>> subscribers;
};

} // namespace Horizon
