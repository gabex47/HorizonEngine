#include "Application.h"

#include "Horizon/EngineInfo.h"
#include "LuauVM.h"
#include "core/Engine.h"
#include "editor/EditorUI.h"
#include "renderer/Framebuffer.h"
#include "renderer/Input.h"
#include "renderer/Renderer.h"

#include <chrono>
#include <iostream>

namespace Horizon {

int Application::Run()
{
    std::cout << "HorizonEngine v0.1 — Opal" << std::endl;

    Engine engine;
    engine.Init();

    Renderer renderer;
    if (!renderer.Init(kDefaultWindowWidth, kDefaultWindowHeight, kEngineName))
    {
        std::cerr << "Renderer init failed" << std::endl;
        return 1;
    }

    Framebuffer fbo(kDefaultWindowWidth, kDefaultWindowHeight);

    EditorUI& editor = EditorUI::Get();
    editor.Init(renderer.GetWindow());
    renderer.InstallInputCallbacks();
    Scripting::LuauVM::SetEditorUI(&editor);
    Scripting::LuauVM luau;
    engine.CreateDefaultScene();

    bool cursorLocked = true;
    renderer.SetCursorLocked(cursorLocked);
    editor.SetMouseInputEnabled(!cursorLocked);

    using Clock = std::chrono::steady_clock;
    int frames = 0;
    auto lastFpsTime = Clock::now();
    auto lastFrameTime = lastFpsTime;

    while (!renderer.ShouldClose())
    {
        renderer.PollEvents();

        const auto now = Clock::now();
        const std::chrono::duration<float> delta = now - lastFrameTime;
        lastFrameTime = now;
        const float deltaTime = delta.count();

        renderer.Update(deltaTime, cursorLocked);
        if (renderer.GetKey(Key::Tab))
            cursorLocked = false;
        if (renderer.GetKey(Key::Escape))
            cursorLocked = true;
        renderer.SetCursorLocked(cursorLocked);
        editor.SetMouseInputEnabled(!cursorLocked);

        fbo.Bind();
        renderer.RenderScene();
        fbo.Unbind();

        renderer.ClearDefaultFramebuffer();
        editor.BeginFrame();
        editor.Render(fbo);
        editor.EndFrame();
        renderer.SwapBuffers();

        engine.Tick(deltaTime);
        editor.SetFPS(deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f);

        ++frames;
        const std::chrono::duration<double> fpsDelta = now - lastFpsTime;
        if (fpsDelta.count() >= 2.0)
        {
            std::cout << "FPS: " << static_cast<double>(frames) / fpsDelta.count() << std::endl;
            frames = 0;
            lastFpsTime = now;
        }
    }

    Scripting::LuauVM::SetEditorUI(nullptr);
    editor.Shutdown();
    return 0;
}

} // namespace Horizon
