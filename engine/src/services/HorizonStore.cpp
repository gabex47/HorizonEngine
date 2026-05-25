#include "HorizonStore.h"

#include <fstream>
#include <nlohmann/json.hpp>

namespace Horizon {

HorizonStore::HorizonStore()
{
    Load();
}

HorizonStore& HorizonStore::Get()
{
    static HorizonStore store;
    return store;
}

void HorizonStore::SetData(const std::string& key, const std::string& value)
{
    store[key] = value;
    Save();
}

std::string HorizonStore::GetData(const std::string& key)
{
    const auto found = store.find(key);
    return found == store.end() ? "" : found->second;
}

bool HorizonStore::HasKey(const std::string& key)
{
    return store.contains(key);
}

void HorizonStore::RemoveKey(const std::string& key)
{
    store.erase(key);
    Save();
}

void HorizonStore::Save()
{
    nlohmann::json data = store;
    std::ofstream output(filePath);
    if (output)
        output << data.dump(2);
}

void HorizonStore::Load()
{
    std::ifstream input(filePath);
    if (!input)
        return;

    try
    {
        nlohmann::json data;
        input >> data;
        if (data.is_object())
            store = data.get<std::unordered_map<std::string, std::string>>();
    }
    catch (...)
    {
        store.clear();
    }
}

} // namespace Horizon
