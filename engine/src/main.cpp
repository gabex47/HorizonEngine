#include <chrono>
#include <iostream>
#include <memory>

#include "Horizon/EngineInfo.h"
#include "LuauVM.h"
#include "Part.h"
#include "renderer/Renderer.h"
#include "services/LoopService.h"
#include "services/Scene.h"

int main()
{
    std::cout << "HorizonEngine v0.1 — Opal" << std::endl;

    Horizon::Scripting::LuauVM luau;
    if (luau.loadScript("print(\"Luau VM online\")"))
        luau.execute();

    auto part = std::make_shared<Horizon::Part>();
    part->Name = "TestCube";
    part->Position = {0.0f, 0.0f, 0.0f};
    part->Size = {1.0f, 1.0f, 1.0f};
    part->Color = {255, 255, 255};
    Horizon::Scene::Get().AddChild(part);

    Horizon::Renderer renderer;
    if (!renderer.Init(Horizon::kDefaultWindowWidth, Horizon::kDefaultWindowHeight, Horizon::kEngineName))
    {
        std::cerr << "Renderer init failed" << std::endl;
        return 1;
    }

    using Clock = std::chrono::steady_clock;
    int frames = 0;
    auto lastFpsTime = Clock::now();
    auto lastFrameTime = lastFpsTime;

    while (!renderer.ShouldClose())
    {
        const auto now = Clock::now();
        const std::chrono::duration<float> delta = now - lastFrameTime;
        lastFrameTime = now;

        renderer.PollEvents();
        Horizon::LoopService::Get().Tick(delta.count());
        renderer.RenderScene();

        ++frames;
        const std::chrono::duration<double> fpsDelta = now - lastFpsTime;
        if (fpsDelta.count() >= 2.0)
        {
            std::cout << "FPS: " << static_cast<double>(frames) / fpsDelta.count() << std::endl;
            frames = 0;
            lastFpsTime = now;
        }
    }

    return 0;
}
