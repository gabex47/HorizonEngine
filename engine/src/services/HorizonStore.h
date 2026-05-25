#pragma once

#include <string>
#include <unordered_map>

namespace Horizon {

class HorizonStore final {
public:
    void SetData(const std::string& key, const std::string& value);
    std::string GetData(const std::string& key);
    bool HasKey(const std::string& key);
    void RemoveKey(const std::string& key);
    static HorizonStore& Get();

private:
    HorizonStore();

    std::unordered_map<std::string, std::string> store;
    std::string filePath = "horizonstore.json";
    void Save();
    void Load();
};

} // namespace Horizon
