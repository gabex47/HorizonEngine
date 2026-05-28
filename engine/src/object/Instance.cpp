#include "Instance.h"

#include <algorithm>

namespace Horizon {

void Instance::SetParent(std::shared_ptr<Instance> parent)
{
    if (Parent && Parent.get() != parent.get())
    {
        auto& siblings = Parent->Children;
        siblings.erase(std::remove_if(siblings.begin(), siblings.end(),
                           [this](const auto& child) { return child.get() == this; }),
            siblings.end());
    }

    Parent = std::move(parent);
    if (!Parent)
        return;

    const auto exists = std::any_of(Parent->Children.begin(), Parent->Children.end(),
        [this](const auto& child) { return child.get() == this; });

    if (!exists)
        Parent->Children.push_back(shared_from_this());
}

void Instance::RemoveChild(std::shared_ptr<Instance> child)
{
    if (!child)
        return;

    const auto oldSize = Children.size();
    Children.erase(std::remove_if(Children.begin(), Children.end(),
                       [&child](const auto& existing) { return existing.get() == child.get(); }),
        Children.end());

    if (Children.size() != oldSize && child->Parent.get() == this)
        child->Parent.reset();
}

std::shared_ptr<Instance> Instance::FindFirstChild(const std::string& name)
{
    const auto found = std::find_if(Children.begin(), Children.end(),
        [&name](const auto& child) { return child && child->Name == name; });
    return found == Children.end() ? nullptr : *found;
}

std::vector<std::shared_ptr<Instance>> Instance::GetChildren()
{
    return Children;
}

std::string Instance::GetClass()
{
    return "Instance";
}

} // namespace Horizon
