#include "Engine.h"

#include "Part.h"
#include "services/LoopService.h"
#include "services/Scene.h"
#include "services/SoundService.h"
#include "services/TweenService.h"

#include <memory>

namespace Horizon {

bool Engine::Init()
{
    SoundService::Get().Init();
    LoopService::Get().BindToHeartbeat("TweenService", [](float dt) {
        TweenService::Get().Tick(dt);
    });
    return true;
}

void Engine::CreateDefaultScene()
{
    auto part = std::make_shared<Part>();
    part->Name = "TestCube";
    part->Position = {0.0f, 0.0f, 0.0f};
    part->Size = {1.0f, 1.0f, 1.0f};
    part->Color = {255, 255, 255};
    Scene::Get().AddChild(part);
}

void Engine::Tick(float deltaTime)
{
    LoopService::Get().Tick(deltaTime);
}

} // namespace Horizon
