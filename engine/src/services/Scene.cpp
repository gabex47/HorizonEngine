#include "Scene.h"

namespace Horizon {

Scene::Scene()
    : root_(std::make_shared<Instance>())
{
    root_->Name = "Scene";
}

Scene& Scene::Get()
{
    static Scene scene;
    return scene;
}

std::vector<std::shared_ptr<Instance>> Scene::GetChildren()
{
    return root_->GetChildren();
}

std::shared_ptr<Instance> Scene::FindFirstChild(const std::string& name)
{
    return root_->FindFirstChild(name);
}

void Scene::AddChild(const std::shared_ptr<Instance>& instance)
{
    if (instance)
        instance->SetParent(root_);
}

std::shared_ptr<Instance> Scene::GetRoot()
{
    return root_;
}

} // namespace Horizon
