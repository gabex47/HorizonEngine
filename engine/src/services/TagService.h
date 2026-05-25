#pragma once

#include "../Instance.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Horizon {

class TagService {
public:
    void AddTag(std::shared_ptr<Instance> instance, const std::string& tag);
    void RemoveTag(std::shared_ptr<Instance> instance, const std::string& tag);
    bool HasTag(std::shared_ptr<Instance> instance, const std::string& tag);
    std::vector<std::shared_ptr<Instance>> GetTagged(const std::string& tag);
    static TagService& Get();

private:
    TagService() = default;

    std::unordered_map<std::string, std::vector<std::shared_ptr<Instance>>> tagMap;
};

} // namespace Horizon
