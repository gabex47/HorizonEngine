#include "EditorUI.h"

#include "Part.h"
#include "editor/EditorActions.h"
#include "object/Script.h"
#include "object/SpotLight.h"
#include "services/Scene.h"
#include "services/SharedStorage.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <imgui.h>

namespace {
constexpr ImVec4 kAccentColor(0.31f, 0.55f, 0.98f, 1.0f);

const char* iconForClass(const std::string& className)
{
    if (className == "Part") return "⬛ ";
    if (className == "ServerScript") return "🖥 ";
    if (className == "LocalScript") return "💻 ";
    if (className == "ModuleScript") return "📄 ";
    if (className == "Folder") return "📁 ";
    if (className == "HorizonEvent") return "⚡ ";
    if (className == "SpotLight") return "💡 ";
    return "•  ";
}

void copyToBuffer(std::array<char, 128>& buffer, const std::string& value)
{
    buffer.fill('\0');
    std::strncpy(buffer.data(), value.c_str(), buffer.size() - 1);
}

void setSelectedHeaderColor(bool selected)
{
    if (!selected)
        return;
    ImGui::PushStyleColor(ImGuiCol_Header, kAccentColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, kAccentColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, kAccentColor);
}

void popSelectedHeaderColor(bool selected)
{
    if (selected)
        ImGui::PopStyleColor(3);
}

void colorEdit(const char* label, Horizon::Color3& color)
{
    float value[3] = {color.R / 255.0f, color.G / 255.0f, color.B / 255.0f};
    if (ImGui::ColorEdit3(label, value))
    {
        color.R = static_cast<uint8_t>(std::clamp(value[0], 0.0f, 1.0f) * 255.0f);
        color.G = static_cast<uint8_t>(std::clamp(value[1], 0.0f, 1.0f) * 255.0f);
        color.B = static_cast<uint8_t>(std::clamp(value[2], 0.0f, 1.0f) * 255.0f);
    }
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

    const auto renderService = [this](const char* icon, const std::shared_ptr<Instance>& root) {
        if (!root)
            return;

        const auto selected = selectedInstance.lock();
        const bool isSelected = selected && selected.get() == root.get();
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_SpanAvailWidth;
        if (isSelected)
            flags |= ImGuiTreeNodeFlags_Selected;

        const std::string label = std::string(icon) + root->Name + "##service_" +
            std::to_string(reinterpret_cast<uintptr_t>(root.get()));
        setSelectedHeaderColor(isSelected);
        const bool open = ImGui::TreeNodeEx(label.c_str(), flags);
        popSelectedHeaderColor(isSelected);

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            selectedInstance = root;
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            selectedInstance = root;
            contextInstance = root;
            ImGui::OpenPopup("InstanceContext");
        }
        if (open)
        {
            for (const auto& child : root->GetChildren())
                RenderInstanceTree(child, 1);
            ImGui::TreePop();
        }
    };

    renderService("🌐 ", Scene::Get().GetRoot());
    renderService("📦 ", SharedStorage::Get().GetRoot());
    renderService("⚙️ ", EditorActions::GetServerScriptsRoot());
    RenderInstanceContextPopup();
    RenderRenameModal();
    ImGui::End();
}

void EditorUI::RenderInstanceTree(std::shared_ptr<Instance> instance, int depth)
{
    if (!instance)
        return;

    const auto selected = selectedInstance.lock();
    const bool isSelected = selected && selected.get() == instance.get();
    const auto children = instance->GetChildren();
    const bool hasChildren = !children.empty();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
    flags |= hasChildren ? ImGuiTreeNodeFlags_OpenOnArrow :
        (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    const std::string className = instance->GetClass();
    const std::string label = std::string(iconForClass(className)) + instance->Name + " (" + className + ")##" +
        std::to_string(depth) + "_" + std::to_string(reinterpret_cast<uintptr_t>(instance.get()));

    setSelectedHeaderColor(isSelected);
    const bool open = ImGui::TreeNodeEx(label.c_str(), flags);
    popSelectedHeaderColor(isSelected);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        selectedInstance = instance;
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        selectedInstance = instance;
        contextInstance = instance;
        ImGui::OpenPopup("InstanceContext");
    }

    if (hasChildren && open)
    {
        for (const auto& child : children)
            RenderInstanceTree(child, depth + 1);
        ImGui::TreePop();
    }
}

void EditorUI::RenderInstanceContextPopup()
{
    if (!ImGui::BeginPopup("InstanceContext"))
        return;

    auto target = contextInstance.lock();
    if (target && ImGui::BeginMenu("Insert Object"))
    {
        const char* classes[] = {"Part", "Folder", "ServerScript", "LocalScript",
            "ModuleScript", "HorizonEvent", "HorizonFunction", "SpotLight"};
        for (const char* className : classes)
        {
            if (ImGui::MenuItem(className))
                selectedInstance = EditorActions::InsertObject(className, target);
        }
        ImGui::EndMenu();
    }

    if (target && ImGui::MenuItem("Rename"))
    {
        renameInstance = target;
        copyToBuffer(renameBuffer, target->Name);
        pendingRenamePopup = true;
    }

    const bool hasParent = target && target->Parent;
    ImGui::BeginDisabled(!hasParent);
    if (ImGui::MenuItem("Delete"))
    {
        const auto selected = selectedInstance.lock();
        target->Parent->RemoveChild(target);
        if (selected && selected.get() == target.get())
            selectedInstance.reset();
        contextInstance.reset();
    }
    if (ImGui::MenuItem("Duplicate"))
    {
        auto duplicate = EditorActions::CreateInstance(target->GetClass());
        duplicate->Name = target->Name + "_copy";
        duplicate->SetParent(target->Parent);
        selectedInstance = duplicate;
    }
    ImGui::EndDisabled();
    ImGui::EndPopup();
}

void EditorUI::RenderRenameModal()
{
    if (pendingRenamePopup)
    {
        ImGui::OpenPopup("Rename Instance");
        pendingRenamePopup = false;
    }

    bool open = true;
    if (ImGui::BeginPopupModal("Rename Instance", &open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Name", renameBuffer.data(), renameBuffer.size());
        if (ImGui::Button("OK"))
        {
            if (auto target = renameInstance.lock())
                target->Name = renameBuffer.data();
            renameInstance.reset();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            renameInstance.reset();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (!open)
        renameInstance.reset();
}

void EditorUI::RenderProperties()
{
    ImGui::Begin("Properties");
    const auto selected = selectedInstance.lock();
    if (!selected) { ImGui::TextDisabled("No instance selected"); ImGui::End(); return; }

    std::array<char, 128> name{};
    std::strncpy(name.data(), selected->Name.c_str(), name.size() - 1);
    if (ImGui::InputText("Name", name.data(), name.size()))
        selected->Name = name.data();

    const std::string className = selected->GetClass();
    ImGui::TextDisabled("ClassName: %s", className.c_str());
    ImGui::Text("Parent: %s", selected->Parent ? selected->Parent->Name.c_str() : "nil");

    if (auto part = std::dynamic_pointer_cast<Part>(selected))
    {
        ImGui::Separator();
        ImGui::TextDisabled("--- Transform ---");
        ImGui::DragFloat3("Position", &part->Position.X, 0.1f);
        ImGui::DragFloat3("Rotation", &part->Rotation.X, 0.1f);
        ImGui::DragFloat3("Size", &part->Size.X, 0.1f, 0.1f, 1000.0f);
        part->Size.X = std::max(part->Size.X, 0.1f);
        part->Size.Y = std::max(part->Size.Y, 0.1f);
        part->Size.Z = std::max(part->Size.Z, 0.1f);

        ImGui::Separator();
        ImGui::TextDisabled("--- Appearance ---");
        colorEdit("Color", part->Color);
        ImGui::SliderFloat("Transparency", &part->Transparency, 0.0f, 1.0f);
        const char* materials[] = {"SmoothPlastic", "Neon", "Metal", "Grass", "Wood", "Brick"};
        int materialIndex = 0;
        for (int i = 0; i < 6; ++i)
            if (part->Material == materials[i])
                materialIndex = i;
        if (ImGui::Combo("Material", &materialIndex, materials, 6))
            part->Material = materials[materialIndex];

        ImGui::Separator();
        ImGui::TextDisabled("--- Behavior ---");
        ImGui::Checkbox("Anchored", &part->Anchored);
        ImGui::Checkbox("CanCollide", &part->CanCollide);
    }

    if (auto script = std::dynamic_pointer_cast<Script>(selected))
    {
        ImGui::Separator();
        ImGui::Checkbox("Enabled", &script->Enabled);
        std::array<char, 65536> source{};
        std::strncpy(source.data(), script->Source.c_str(), source.size() - 1);
        if (ImGui::InputTextMultiline("Source", source.data(), source.size(), ImVec2(-1.0f, 400.0f)))
            script->Source = source.data();
    }

    if (auto light = std::dynamic_pointer_cast<SpotLight>(selected))
    {
        ImGui::Separator();
        ImGui::Checkbox("Enabled", &light->Enabled);
        ImGui::SliderFloat("Brightness", &light->Brightness, 0.0f, 5.0f);
        ImGui::SliderFloat("Range", &light->Range, 0.0f, 100.0f);
        ImGui::SliderFloat("Angle", &light->Angle, 0.0f, 90.0f);
        colorEdit("Color", light->Color);
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
