#include "HorizonEvent.h"

#include <utility>

namespace Horizon {

void HorizonEvent::FireServer(std::vector<std::string> args)
{
    if (serverCallback)
        serverCallback(args);
}

void HorizonEvent::FireClient(std::vector<std::string> args)
{
    if (clientCallback)
        clientCallback(args);
}

void HorizonEvent::FireAllClients(std::vector<std::string> args)
{
    if (clientCallback)
        clientCallback(args);
}

void HorizonEvent::OnServerEvent(Callback cb)
{
    serverCallback = std::move(cb);
}

void HorizonEvent::OnClientEvent(Callback cb)
{
    clientCallback = std::move(cb);
}

std::string HorizonEvent::GetClass()
{
    return "HorizonEvent";
}

} // namespace Horizon
