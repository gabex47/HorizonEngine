#include "EditorUI.h"

#include "Part.h"
#include "services/Scene.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <imgui.h>

namespace {
constexpr ImVec4 kAccentColor(0.31f, 0.55f, 0.98f, 1.0f);
const char* iconForClass(const std::string& className)
{
    if (className == "Part") return "⬛ ";
    if (className == "Folder") return "📁 ";
    if (className == "HorizonEvent") return "⚡ ";
    return "• ";
}
} // namespace

namespace Horizon {

void EditorUI::RenderMenuBar()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene")) PushLog("New Scene selected");
            if (ImGui::MenuItem("Open Scene")) PushLog("Open Scene selected");
            if (ImGui::MenuItem("Save Scene")) PushLog("Save Scene selected");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo")) PushLog("Undo selected");
            if (ImGui::MenuItem("Redo")) PushLog("Redo selected");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Toggle Explorer", nullptr, &showExplorer);
            ImGui::MenuItem("Toggle Properties", nullptr, &showProperties);
            ImGui::MenuItem("Toggle Console", nullptr, &showConsole);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) { if (ImGui::MenuItem("About")) showAbout = true; ImGui::EndMenu(); }
        ImGui::EndMainMenuBar();
    }
    if (showAbout) ImGui::OpenPopup("About HorizonEngine");
    if (ImGui::BeginPopupModal("About HorizonEngine", &showAbout, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("HorizonEngine v0.1 by Opal");
        if (ImGui::Button("OK")) showAbout = false;
        ImGui::EndPopup();
    }
}

void EditorUI::RenderExplorer()
{
    ImGui::Begin("Explorer");
    ImGui::TextColored(kAccentColor, "HorizonEngine");
    if (ImGui::TreeNode("Scene")) {
        const auto selected = selectedInstance.lock();
        for (const auto& instance : Scene::Get().GetChildren()) {
            if (!instance) continue;
            const bool isSelected = selected && selected.get() == instance.get();
            const std::string className = instance->GetClass();
            const std::string label = std::string(iconForClass(className)) + instance->Name +
                " (" + className + ")##" + std::to_string(reinterpret_cast<uintptr_t>(instance.get()));
            if (isSelected) ImGui::PushStyleColor(ImGuiCol_Header, kAccentColor);
            if (ImGui::Selectable(label.c_str(), isSelected)) selectedInstance = instance;
            if (isSelected) ImGui::PopStyleColor();
        }
        ImGui::TreePop();
    }
    ImGui::End();
}

void EditorUI::RenderProperties()
{
    ImGui::Begin("Properties");
    const auto selected = selectedInstance.lock();
    if (!selected) { ImGui::TextDisabled("No instance selected"); ImGui::End(); return; }
    char name[128] = {};
    std::strncpy(name, selected->Name.c_str(), sizeof(name) - 1);
    if (ImGui::InputText("Name", name, sizeof(name))) selected->Name = name;
    const std::string className = selected->GetClass();
    ImGui::Text("ClassName: %s", className.c_str());
    if (auto part = std::dynamic_pointer_cast<Part>(selected)) {
        ImGui::DragFloat3("Position", &part->Position.X, 0.1f);
        ImGui::DragFloat3("Size", &part->Size.X, 0.1f, 0.01f, 100.0f);
        float color[3] = {part->Color.R / 255.0f, part->Color.G / 255.0f, part->Color.B / 255.0f};
        if (ImGui::ColorEdit3("Color", color)) {
            part->Color.R = static_cast<uint8_t>(std::clamp(color[0], 0.0f, 1.0f) * 255.0f);
            part->Color.G = static_cast<uint8_t>(std::clamp(color[1], 0.0f, 1.0f) * 255.0f);
            part->Color.B = static_cast<uint8_t>(std::clamp(color[2], 0.0f, 1.0f) * 255.0f);
        }
    }
    ImGui::End();
}

void EditorUI::RenderConsole()
{
    ImGui::Begin("Console");
    if (ImGui::Button("Clear")) consoleLogs.clear();
    ImGui::Separator();
    for (const auto& line : consoleLogs) {
        ImVec4 color(0.85f, 0.85f, 0.85f, 1.0f);
        if (line.rfind("[WARN]", 0) == 0) color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
        else if (line.rfind("[ERROR]", 0) == 0) color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        ImGui::TextColored(color, "%s", line.c_str());
    }
    if (scrollConsoleToBottom) { ImGui::SetScrollHereY(1.0f); scrollConsoleToBottom = false; }
    ImGui::End();
}

} // namespace Horizon
