#include "Application.h"

#include "Horizon/EngineInfo.h"
#include "LuauVM.h"
#include "core/Engine.h"
#include "renderer/Renderer.h"

#include <chrono>
#include <iostream>

namespace Horizon {

int Application::Run()
{
    std::cout << "HorizonEngine v0.1 — Opal" << std::endl;

    Engine engine;
    engine.Init();

    Scripting::LuauVM luau;
    engine.CreateDefaultScene();

    Renderer renderer;
    if (!renderer.Init(kDefaultWindowWidth, kDefaultWindowHeight, kEngineName))
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
        engine.Tick(delta.count());
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

} // namespace Horizon
