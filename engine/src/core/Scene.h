#pragma once

#include "../Instance.h"

namespace Horizon {

class Scene final {
public:
    static Scene& Get();

    std::vector<std::shared_ptr<Instance>> GetChildren();
    std::shared_ptr<Instance> FindFirstChild(const std::string& name);
    void AddChild(const std::shared_ptr<Instance>& instance);
    std::shared_ptr<Instance> GetRoot();

private:
    Scene();

    std::shared_ptr<Instance> root_;
};

} // namespace Horizon
