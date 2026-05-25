#pragma once

#include "../Instance.h"

namespace Horizon {

class SharedStorage final {
public:
    static SharedStorage& Get();

    std::vector<std::shared_ptr<Instance>> GetChildren();
    std::shared_ptr<Instance> FindFirstChild(const std::string& name);
    void AddChild(const std::shared_ptr<Instance>& instance);
    std::shared_ptr<Instance> GetRoot();

private:
    SharedStorage();

    std::shared_ptr<Instance> root_;
};

} // namespace Horizon
