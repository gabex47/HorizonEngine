#include "SharedStorage.h"

namespace Horizon {

SharedStorage::SharedStorage()
    : root_(std::make_shared<Instance>())
{
    root_->Name = "SharedStorage";
}

SharedStorage& SharedStorage::Get()
{
    static SharedStorage storage;
    return storage;
}

std::vector<std::shared_ptr<Instance>> SharedStorage::GetChildren()
{
    return root_->GetChildren();
}

std::shared_ptr<Instance> SharedStorage::FindFirstChild(const std::string& name)
{
    return root_->FindFirstChild(name);
}

void SharedStorage::AddChild(const std::shared_ptr<Instance>& instance)
{
    if (instance)
        instance->SetParent(root_);
}

std::shared_ptr<Instance> SharedStorage::GetRoot()
{
    return root_;
}

} // namespace Horizon
