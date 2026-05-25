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
