#pragma once

#include <string>
#include <vector>

namespace Horizon {

class ServerScripts final {
public:
    static ServerScripts& Get();

    void RegisterScript(const std::string& path);
    const std::vector<std::string>& GetScripts() const;

private:
    ServerScripts() = default;

    std::vector<std::string> scripts_;
};

} // namespace Horizon
