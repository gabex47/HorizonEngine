#include "ServerScripts.h"

namespace Horizon {

ServerScripts& ServerScripts::Get()
{
    static ServerScripts scripts;
    return scripts;
}

void ServerScripts::RegisterScript(const std::string& path)
{
    scripts_.push_back(path);
}

const std::vector<std::string>& ServerScripts::GetScripts() const
{
    return scripts_;
}

} // namespace Horizon
