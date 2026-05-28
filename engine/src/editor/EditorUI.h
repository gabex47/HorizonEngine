#pragma once

#include "Instance.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

struct GLFWwindow;

namespace Horizon {

class Framebuffer;

class EditorUI final {
public:
    static EditorUI& Get();

    void Init(GLFWwindow* window);
    void BeginFrame();
    void Render(Framebuffer& fbo);
    void EndFrame();
    void Shutdown();
    void PushLog(const std::string& message);
    void SetFPS(float fps);
    void SetMouseInputEnabled(bool enabled);

private:
    void RenderMenuBar();
    void RenderExplorer();
    void RenderInstanceTree(std::shared_ptr<Instance> instance, int depth);
    void RenderViewport(Framebuffer& fbo);
    void RenderProperties();
    void RenderConsole();
    void RenderInstanceContextPopup();
    void RenderRenameModal();

    std::weak_ptr<Instance> selectedInstance;
    std::weak_ptr<Instance> contextInstance;
    std::weak_ptr<Instance> renameInstance;
    std::array<char, 128> renameBuffer{};
    std::vector<std::string> consoleLogs;
    float currentFPS = 0.0f;
    bool scrollConsoleToBottom = false;
    bool showExplorer = true;
    bool showProperties = true;
    bool showConsole = true;
    bool showAbout = false;
    bool pendingRenamePopup = false;
    bool mouseInputEnabled = true;
    bool initialized = false;
};

} // namespace Horizon
