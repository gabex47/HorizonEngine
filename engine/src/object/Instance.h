#pragma once

#include <memory>
#include <string>
#include <vector>

namespace Horizon {

class Instance : public std::enable_shared_from_this<Instance> {
public:
    std::string Name;
    std::shared_ptr<Instance> Parent;
    std::vector<std::shared_ptr<Instance>> Children;

    virtual ~Instance() = default;

    void SetParent(std::shared_ptr<Instance> parent);
    void RemoveChild(std::shared_ptr<Instance> child);
    std::shared_ptr<Instance> FindFirstChild(const std::string& name);
    std::vector<std::shared_ptr<Instance>> GetChildren();
    virtual std::string GetClass();
};

} // namespace Horizon
