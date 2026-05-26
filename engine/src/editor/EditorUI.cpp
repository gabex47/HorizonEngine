#include "EditorUI.h"

#include "renderer/Framebuffer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace { constexpr ImVec4 kAccentColor(0.31f, 0.55f, 0.98f, 1.0f); }

namespace Horizon {

EditorUI& EditorUI::Get() { static EditorUI editor; return editor; }

void EditorUI::Init(GLFWwindow* window)
{
    if (!window)
        return;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f; style.FrameRounding = 3.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.26f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.29f, 0.29f, 0.38f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = kAccentColor;
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.26f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.19f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.13f, 0.17f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = kAccentColor;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    initialized = true;
}

void EditorUI::BeginFrame()
{
    if (!initialized)
        return;

    ImGuiIO& io = ImGui::GetIO();
    mouseInputEnabled ? io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse : io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0)); ImGui::SetNextWindowSize(io.DisplaySize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGui::Begin("DockSpace", nullptr, flags);
    ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr) {
        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, io.DisplaySize);
        ImGuiID dockMain = dockspaceId;
        ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.18f, nullptr, &dockMain);
        ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.22f, nullptr, &dockMain);
        ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.25f, nullptr, &dockMain);
        ImGui::DockBuilderDockWindow("Explorer", dockLeft);
        ImGui::DockBuilderDockWindow("Viewport", dockMain);
        ImGui::DockBuilderDockWindow("Properties", dockRight);
        ImGui::DockBuilderDockWindow("Console", dockBottom);
        ImGui::DockBuilderFinish(dockspaceId);
    }
    ImGui::DockSpace(dockspaceId);
    ImGui::End();
}

void EditorUI::Render(Framebuffer& fbo)
{
    RenderMenuBar();
    if (showExplorer) RenderExplorer();
    RenderViewport(fbo);
    if (showProperties) RenderProperties();
    if (showConsole) RenderConsole();
}

void EditorUI::EndFrame()
{
    if (!initialized)
        return;

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorUI::Shutdown()
{
    if (!initialized)
        return;

    initialized = false;
    selectedInstance.reset();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorUI::PushLog(const std::string& message)
{
    if (!initialized)
        return;

    consoleLogs.push_back(message);
    scrollConsoleToBottom = true;
}
void EditorUI::SetFPS(float fps) { currentFPS = fps; }
void EditorUI::SetMouseInputEnabled(bool enabled) { mouseInputEnabled = enabled; }

void EditorUI::RenderViewport(Framebuffer& fbo)
{
    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar);
    ImVec2 size = ImGui::GetContentRegionAvail();
    if (size.x > 1.0f && size.y > 1.0f) {
        const int width = static_cast<int>(size.x), height = static_cast<int>(size.y);
        if (width != fbo.GetWidth() || height != fbo.GetHeight()) fbo.Resize(width, height);
        const ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::Image((ImTextureID)(intptr_t)fbo.GetTexture(), size, ImVec2(0, 1), ImVec2(1, 0));
        ImGui::SetCursorPos(ImVec2(cursor.x + 10.0f, cursor.y + 10.0f));
        ImGui::TextUnformatted("HorizonEngine v0.1 | Opal"); ImGui::Text("FPS: %.1f", currentFPS);
        ImGui::TextUnformatted("WASD to move | Mouse to look");
        ImGui::TextUnformatted("Tab — unlock cursor | Escape — lock cursor");
    }
    ImGui::End();
}

} // namespace Horizon
