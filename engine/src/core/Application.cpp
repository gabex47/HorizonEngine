#include "Application.h"

#include "Horizon/EngineInfo.h"
#include "LuauVM.h"
#include "core/Engine.h"
#include "core/EngineMode.h"
#include "editor/EditorUI.h"
#include "Part.h"
#include "renderer/Framebuffer.h"
#include "renderer/Input.h"
#include "renderer/Renderer.h"
#include "services/Scene.h"

#include <chrono>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <memory>

namespace Horizon {

namespace {

std::shared_ptr<Part> findPlayerPart()
{
    return std::dynamic_pointer_cast<Part>(Scene::Get().FindFirstChild("Player"));
}

void updatePlayerMovement(Renderer& renderer, Part& playerPart, float deltaTime)
{
    glm::vec3 forward = renderer.GetPlayCameraForwardDir();
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 moveDir{0.0f, 0.0f, 0.0f};

    if (renderer.GetKey(Key::W)) moveDir += forward;
    if (renderer.GetKey(Key::S)) moveDir -= forward;
    if (renderer.GetKey(Key::A)) moveDir -= right;
    if (renderer.GetKey(Key::D)) moveDir += right;

    if (glm::length(moveDir) > 0.0f)
        moveDir = glm::normalize(moveDir);

    constexpr float speed = 8.0f;
    playerPart.Position.X += moveDir.x * speed * deltaTime;
    playerPart.Position.Z += moveDir.z * speed * deltaTime;
    playerPart.Transparency = renderer.GetPlayCameraDistance() <= 2.0f ? 1.0f : 0.0f;
    renderer.SetPlayCameraTarget(glm::vec3(playerPart.Position.X, playerPart.Position.Y, playerPart.Position.Z));
}

} // namespace

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
    EngineMode previousMode = gEngineMode;
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

        if (previousMode != gEngineMode)
        {
            if (gEngineMode == EngineMode::Play)
            {
                cursorLocked = false;
                renderer.ResetPlayCamera();
                renderer.SetCursorLocked(false);
                editor.SetMouseInputEnabled(true);
            }
            else
            {
                cursorLocked = true;
                renderer.SetCursorLocked(true);
                editor.SetMouseInputEnabled(false);
            }
            previousMode = gEngineMode;
        }

        if (gEngineMode == EngineMode::Edit)
        {
            renderer.Update(deltaTime, cursorLocked);
            if (renderer.GetKey(Key::Tab))
                cursorLocked = false;
            if (renderer.GetKey(Key::Escape))
                cursorLocked = true;
            renderer.SetCursorLocked(cursorLocked);
            editor.SetMouseInputEnabled(!cursorLocked);
        }
        else
        {
            renderer.Update(deltaTime, false);
            if (auto playerPart = findPlayerPart())
                updatePlayerMovement(renderer, *playerPart, deltaTime);
            else
                renderer.SetPlayCameraTarget(glm::vec3(0.0f, 1.0f, 0.0f));
            editor.SetMouseInputEnabled(renderer.GetPlayCameraDistance() > 2.0f);
        }

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
