#include "HorizonFunction.h"

#include <utility>

namespace Horizon {

std::string HorizonFunction::InvokeServer(std::vector<std::string> args)
{
    if (!serverHandler)
        return "";

    return serverHandler(args);
}

void HorizonFunction::OnServerInvoke(Handler handler)
{
    serverHandler = std::move(handler);
}

std::string HorizonFunction::GetClass()
{
    return "HorizonFunction";
}

} // namespace Horizon
