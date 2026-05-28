#include "EditorActions.h"

#include "core/EngineMode.h"
#include "events/HorizonEvent.h"
#include "events/HorizonFunction.h"
#include "object/Folder.h"
#include "object/LocalScript.h"
#include "object/ModuleScript.h"
#include "object/ServerScript.h"
#include "object/SpotLight.h"
#include "services/BindableEvent.h"
#include "services/Scene.h"

#include <utility>

namespace Horizon::EditorActions {

std::shared_ptr<Instance> CreateInstance(const std::string& className)
{
    std::shared_ptr<Instance> instance;
    if (className == "Part")
        instance = std::make_shared<Part>();
    else if (className == "Folder")
        instance = std::make_shared<Folder>();
    else if (className == "ServerScript")
        instance = std::make_shared<ServerScript>();
    else if (className == "LocalScript")
        instance = std::make_shared<LocalScript>();
    else if (className == "ModuleScript")
        instance = std::make_shared<ModuleScript>();
    else if (className == "HorizonEvent")
        instance = std::make_shared<HorizonEvent>();
    else if (className == "HorizonFunction")
        instance = std::make_shared<HorizonFunction>();
    else if (className == "SpotLight")
        instance = std::make_shared<SpotLight>();
    else if (className == "BindableEvent")
        instance = std::make_shared<BindableEvent>();
    else
        instance = std::make_shared<Instance>();

    instance->Name = className;
    return instance;
}

std::shared_ptr<Instance> InsertObject(const std::string& className, std::shared_ptr<Instance> parent)
{
    auto instance = CreateInstance(className);
    if (parent)
        instance->SetParent(std::move(parent));
    return instance;
}

std::shared_ptr<Instance> GetServerScriptsRoot()
{
    static std::shared_ptr<Instance> root = [] {
        auto instance = std::make_shared<Instance>();
        instance->Name = "ServerScripts";
        return instance;
    }();
    return root;
}

void ResetScene()
{
    if (gEngineMode == EngineMode::Play)
        gEngineMode = EngineMode::Edit;

    auto root = Scene::Get().GetRoot();
    for (const auto& child : root->GetChildren())
        root->RemoveChild(child);

    auto part = std::make_shared<Part>();
    part->Name = "TestCube";
    part->Position = {0.0f, 0.0f, 0.0f};
    part->Size = {1.0f, 1.0f, 1.0f};
    part->Color = {255, 255, 255};
    Scene::Get().AddChild(part);
}

bool EnterPlayMode()
{
    if (gEngineMode == EngineMode::Play)
        return false;

    if (auto player = Scene::Get().FindFirstChild("Player"))
        Scene::Get().GetRoot()->RemoveChild(player);

    auto player = std::make_shared<Part>();
    player->Name = "Player";
    player->Size = {1.0f, 2.0f, 1.0f};
    player->Color = {70, 130, 220};
    player->Position = {0.0f, 1.0f, 0.0f};
    player->Anchored = false;
    Scene::Get().AddChild(player);

    gEngineMode = EngineMode::Play;
    return true;
}

bool ExitPlayMode()
{
    if (auto player = Scene::Get().FindFirstChild("Player"))
        Scene::Get().GetRoot()->RemoveChild(player);

    if (gEngineMode == EngineMode::Edit)
        return false;

    gEngineMode = EngineMode::Edit;
    return true;
}

} // namespace Horizon::EditorActions
