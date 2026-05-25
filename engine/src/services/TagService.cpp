#include "TagService.h"

#include <algorithm>

namespace Horizon {

TagService& TagService::Get()
{
    static TagService service;
    return service;
}

void TagService::AddTag(std::shared_ptr<Instance> instance, const std::string& tag)
{
    if (!instance)
        return;

    auto& tagged = tagMap[tag];
    const auto exists = std::any_of(tagged.begin(), tagged.end(),
        [&instance](const auto& item) { return item.get() == instance.get(); });

    if (!exists)
        tagged.push_back(std::move(instance));
}

void TagService::RemoveTag(std::shared_ptr<Instance> instance, const std::string& tag)
{
    const auto found = tagMap.find(tag);
    if (found == tagMap.end())
        return;

    auto& tagged = found->second;
    tagged.erase(std::remove_if(tagged.begin(), tagged.end(),
                     [&instance](const auto& item) { return !item || item.get() == instance.get(); }),
        tagged.end());

    if (tagged.empty())
        tagMap.erase(found);
}

bool TagService::HasTag(std::shared_ptr<Instance> instance, const std::string& tag)
{
    const auto found = tagMap.find(tag);
    if (!instance || found == tagMap.end())
        return false;

    return std::any_of(found->second.begin(), found->second.end(),
        [&instance](const auto& item) { return item && item.get() == instance.get(); });
}

std::vector<std::shared_ptr<Instance>> TagService::GetTagged(const std::string& tag)
{
    const auto found = tagMap.find(tag);
    if (found == tagMap.end())
        return {};

    return found->second;
}

} // namespace Horizon
