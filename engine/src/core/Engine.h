#pragma once

namespace Horizon {

class Engine final {
public:
    bool Init();
    void CreateDefaultScene();
    void Tick(float deltaTime);
};

} // namespace Horizon
