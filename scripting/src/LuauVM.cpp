#include "LuauVM.h"

#include "Instance.h"
#include "Part.h"
#include "object/Folder.h"
#include "object/LocalScript.h"
#include "object/ModuleScript.h"
#include "object/ServerScript.h"
#include "object/SpotLight.h"
#include "events/HorizonEvent.h"
#include "events/HorizonFunction.h"
#include "editor/EditorUI.h"
#include "services/BindableEvent.h"
#include "services/HorizonStore.h"
#include "services/LoopService.h"
#include "services/MessagingService.h"
#include "services/Scene.h"
#include "services/ServerScripts.h"
#include "services/SharedStorage.h"
#include "services/SoundService.h"
#include "services/TagService.h"
#include "services/TweenService.h"

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <Luau/Compiler.h>

#include <lua.h>
#include <lualib.h>

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr const char* kInstanceMetatable = "Horizon.Instance";
constexpr const char* kServerScriptsMetatable = "Horizon.ServerScripts";
constexpr const char* kLoopServiceMetatable = "Horizon.LoopService";
constexpr const char* kHorizonStoreMetatable = "Horizon.HorizonStore";
constexpr const char* kMessagingServiceMetatable = "Horizon.MessagingService";
constexpr const char* kTweenMetatable = "Horizon.Tween";
constexpr const char* kTweenServiceMetatable = "Horizon.TweenService";
constexpr const char* kSoundServiceMetatable = "Horizon.SoundService";
constexpr const char* kTagServiceMetatable = "Horizon.TagService";
constexpr const char* kServicesSmokeTest = R"(
-- TweenService test
local part = Instance.new("Part")
part.Name = "TweenTest"
part:SetParent(Scene)
local tween = TweenService:Create(part, {Duration=1.0, EasingStyle="Sine", EasingDirection="Out"}, {PositionX=5.0})
tween:Play()
print("Tween created")

-- SoundService test
SoundService:SetVolume(1.0)
SoundService:PlaySound("test.wav")
print("SoundService online")

-- BindableEvent test
local be = Instance.new("BindableEvent")
be:Connect(function(args)
  print("BindableEvent fired: " .. args[1])
end)
be:Fire({"hello"})

-- TagService test
local p = Instance.new("Part")
TagService:AddTag(p, "Enemy")
print(tostring(TagService:HasTag(p, "Enemy")))
print("TagService online")
)";

using InstanceStore = std::vector<std::shared_ptr<Horizon::Instance>>;
using CallbackRefStore = std::vector<int>;
using TweenHandle = std::shared_ptr<Horizon::Tween>;

Horizon::EditorUI* editorRef = nullptr;

int luaPrint(lua_State* L)
{
    std::string message;
    const int count = lua_gettop(L);
    for (int i = 1; i <= count; ++i)
    {
        size_t length = 0;
        const char* value = luaL_tolstring(L, i, &length);
        if (i > 1)
            message += "\t";
        if (value)
            message.append(value, length);
        lua_pop(L, 1);
    }

    std::cout << message << std::endl;
    if (editorRef)
        editorRef->PushLog(message);
    return 0;
}

void registerPrintHandler(lua_State* L)
{
    lua_pushcfunction(L, luaPrint, "print");
    lua_setglobal(L, "print");
}

Horizon::Instance* checkInstance(lua_State* L, int index)
{
    auto** userdata = static_cast<Horizon::Instance**>(
        luaL_checkudata(L, index, kInstanceMetatable));
    return *userdata;
}

Horizon::HorizonEvent* checkHorizonEvent(lua_State* L, int index)
{
    auto* event = dynamic_cast<Horizon::HorizonEvent*>(checkInstance(L, index));
    if (!event)
        luaL_typeerror(L, index, "HorizonEvent");

    return event;
}

Horizon::HorizonFunction* checkHorizonFunction(lua_State* L, int index)
{
    auto* func = dynamic_cast<Horizon::HorizonFunction*>(checkInstance(L, index));
    if (!func)
        luaL_typeerror(L, index, "HorizonFunction");

    return func;
}

Horizon::BindableEvent* checkBindableEvent(lua_State* L, int index)
{
    auto* event = dynamic_cast<Horizon::BindableEvent*>(checkInstance(L, index));
    if (!event)
        luaL_typeerror(L, index, "BindableEvent");

    return event;
}

TweenHandle* checkTweenHandle(lua_State* L, int index)
{
    return static_cast<TweenHandle*>(luaL_checkudata(L, index, kTweenMetatable));
}

Horizon::Tween* checkTween(lua_State* L, int index)
{
    auto* handle = checkTweenHandle(L, index);
    if (!handle || !*handle)
        luaL_typeerror(L, index, "Tween");

    return handle->get();
}

std::shared_ptr<Horizon::Instance> sharedInstance(lua_State* L, int index)
{
    return checkInstance(L, index)->shared_from_this();
}

void pushInstance(lua_State* L, Horizon::Instance* instance)
{
    auto** userdata = static_cast<Horizon::Instance**>(
        lua_newuserdata(L, sizeof(Horizon::Instance*)));
    *userdata = instance;
    luaL_getmetatable(L, kInstanceMetatable);
    lua_setmetatable(L, -2);
}

void pushTween(lua_State* L, std::shared_ptr<Horizon::Tween> tween)
{
    auto* userdata = static_cast<TweenHandle*>(lua_newuserdata(L, sizeof(TweenHandle)));
    new (userdata) TweenHandle(std::move(tween));
    luaL_getmetatable(L, kTweenMetatable);
    lua_setmetatable(L, -2);
}

Horizon::TweenInfo readTweenInfo(lua_State* L, int index)
{
    Horizon::TweenInfo info;
    luaL_checktype(L, index, LUA_TTABLE);
    const int table = lua_absindex(L, index);

    lua_getfield(L, table, "Duration");
    if (lua_isnumber(L, -1))
        info.Duration = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, table, "EasingStyle");
    if (const char* value = lua_tostring(L, -1))
        info.EasingStyle = value;
    lua_pop(L, 1);

    lua_getfield(L, table, "EasingDirection");
    if (const char* value = lua_tostring(L, -1))
        info.EasingDirection = value;
    lua_pop(L, 1);

    lua_getfield(L, table, "RepeatCount");
    if (lua_isnumber(L, -1))
        info.RepeatCount = static_cast<int>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, table, "Reverses");
    if (lua_isboolean(L, -1))
        info.Reverses = lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);
    return info;
}

std::unordered_map<std::string, float> readFloatGoals(lua_State* L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);
    const int table = lua_absindex(L, index);
    std::unordered_map<std::string, float> goals;

    lua_pushnil(L);
    while (lua_next(L, table) != 0)
    {
        const char* key = lua_tostring(L, -2);
        if (key && lua_isnumber(L, -1))
            goals[key] = static_cast<float>(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }

    return goals;
}

std::vector<std::string> readStringArgs(lua_State* L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);

    const int tableIndex = lua_absindex(L, index);
    const int length = lua_objlen(L, tableIndex);
    std::vector<std::string> args;
    args.reserve(static_cast<size_t>(length));

    for (int i = 1; i <= length; ++i)
    {
        lua_rawgeti(L, tableIndex, i);
        args.emplace_back(luaL_checkstring(L, -1));
        lua_pop(L, 1);
    }

    return args;
}

void pushStringArgs(lua_State* L, const std::vector<std::string>& args)
{
    lua_createtable(L, static_cast<int>(args.size()), 0);

    int index = 1;
    for (const auto& arg : args)
    {
        lua_pushstring(L, arg.c_str());
        lua_rawseti(L, -2, index++);
    }
}

void logLuaCallbackError(lua_State* L, const char* name)
{
    const char* message = lua_tostring(L, -1);
    std::cerr << name << " failed: " << (message ? message : "unknown error") << std::endl;
    lua_pop(L, 1);
}

void callLuaCallback(lua_State* L, int ref, const std::vector<std::string>& args)
{
    lua_getref(L, ref);
    pushStringArgs(L, args);

    if (lua_pcall(L, 1, 0, 0) != LUA_OK)
        logLuaCallbackError(L, "Luau callback");
}

void callLuaNumberCallback(lua_State* L, int ref, float value)
{
    lua_getref(L, ref);
    lua_pushnumber(L, value);

    if (lua_pcall(L, 1, 0, 0) != LUA_OK)
        logLuaCallbackError(L, "Luau callback");
}

void callLuaStringCallback(lua_State* L, int ref, const std::string& value)
{
    lua_getref(L, ref);
    lua_pushstring(L, value.c_str());

    if (lua_pcall(L, 1, 0, 0) != LUA_OK)
        logLuaCallbackError(L, "Luau callback");
}

std::string callLuaHandler(lua_State* L, int ref, const std::vector<std::string>& args)
{
    lua_getref(L, ref);
    pushStringArgs(L, args);

    if (lua_pcall(L, 1, 1, 0) != LUA_OK)
    {
        logLuaCallbackError(L, "Luau handler");
        return "";
    }

    const char* result = lua_tostring(L, -1);
    std::string value = result ? result : "";
    lua_pop(L, 1);
    return value;
}

CallbackRefStore* callbackRefs(lua_State* L)
{
    return static_cast<CallbackRefStore*>(lua_tolightuserdata(L, lua_upvalueindex(1)));
}

int retainLuaFunction(lua_State* L, int index)
{
    luaL_checktype(L, index, LUA_TFUNCTION);
    const int ref = lua_ref(L, index);

    if (auto* refs = callbackRefs(L))
        refs->push_back(ref);

    return ref;
}

void pushCallbackMethod(lua_State* L, lua_CFunction fn, const char* debugName)
{
    lua_pushlightuserdata(L, callbackRefs(L));
    lua_pushcclosurek(L, fn, debugName, 1, nullptr);
}

Horizon::ServerScripts* checkServerScripts(lua_State* L, int index)
{
    auto** userdata = static_cast<Horizon::ServerScripts**>(
        luaL_checkudata(L, index, kServerScriptsMetatable));
    return *userdata;
}

void pushServerScripts(lua_State* L, Horizon::ServerScripts* service)
{
    auto** userdata = static_cast<Horizon::ServerScripts**>(
        lua_newuserdata(L, sizeof(Horizon::ServerScripts*)));
    *userdata = service;
    luaL_getmetatable(L, kServerScriptsMetatable);
    lua_setmetatable(L, -2);
}

Horizon::LoopService* checkLoopService(lua_State* L, int index)
{
    auto** userdata = static_cast<Horizon::LoopService**>(
        luaL_checkudata(L, index, kLoopServiceMetatable));
    return *userdata;
}

void pushLoopService(lua_State* L, Horizon::LoopService* service)
{
    auto** userdata = static_cast<Horizon::LoopService**>(
        lua_newuserdata(L, sizeof(Horizon::LoopService*)));
    *userdata = service;
    luaL_getmetatable(L, kLoopServiceMetatable);
    lua_setmetatable(L, -2);
}

Horizon::HorizonStore* checkHorizonStore(lua_State* L, int index)
{
    auto** userdata = static_cast<Horizon::HorizonStore**>(
        luaL_checkudata(L, index, kHorizonStoreMetatable));
    return *userdata;
}

void pushHorizonStore(lua_State* L, Horizon::HorizonStore* service)
{
    auto** userdata = static_cast<Horizon::HorizonStore**>(
        lua_newuserdata(L, sizeof(Horizon::HorizonStore*)));
    *userdata = service;
    luaL_getmetatable(L, kHorizonStoreMetatable);
    lua_setmetatable(L, -2);
}

Horizon::MessagingService* checkMessagingService(lua_State* L, int index)
{
    auto** userdata = static_cast<Horizon::MessagingService**>(
        luaL_checkudata(L, index, kMessagingServiceMetatable));
    return *userdata;
}

void pushMessagingService(lua_State* L, Horizon::MessagingService* service)
{
    auto** userdata = static_cast<Horizon::MessagingService**>(
        lua_newuserdata(L, sizeof(Horizon::MessagingService*)));
    *userdata = service;
    luaL_getmetatable(L, kMessagingServiceMetatable);
    lua_setmetatable(L, -2);
}

Horizon::TweenService* checkTweenService(lua_State* L, int index)
{
    auto** userdata = static_cast<Horizon::TweenService**>(
        luaL_checkudata(L, index, kTweenServiceMetatable));
    return *userdata;
}

void pushTweenService(lua_State* L, Horizon::TweenService* service)
{
    auto** userdata = static_cast<Horizon::TweenService**>(
        lua_newuserdata(L, sizeof(Horizon::TweenService*)));
    *userdata = service;
    luaL_getmetatable(L, kTweenServiceMetatable);
    lua_setmetatable(L, -2);
}

Horizon::SoundService* checkSoundService(lua_State* L, int index)
{
    auto** userdata = static_cast<Horizon::SoundService**>(
        luaL_checkudata(L, index, kSoundServiceMetatable));
    return *userdata;
}

void pushSoundService(lua_State* L, Horizon::SoundService* service)
{
    auto** userdata = static_cast<Horizon::SoundService**>(
        lua_newuserdata(L, sizeof(Horizon::SoundService*)));
    *userdata = service;
    luaL_getmetatable(L, kSoundServiceMetatable);
    lua_setmetatable(L, -2);
}

Horizon::TagService* checkTagService(lua_State* L, int index)
{
    auto** userdata = static_cast<Horizon::TagService**>(
        luaL_checkudata(L, index, kTagServiceMetatable));
    return *userdata;
}

void pushTagService(lua_State* L, Horizon::TagService* service)
{
    auto** userdata = static_cast<Horizon::TagService**>(
        lua_newuserdata(L, sizeof(Horizon::TagService*)));
    *userdata = service;
    luaL_getmetatable(L, kTagServiceMetatable);
    lua_setmetatable(L, -2);
}

int instanceSetParent(lua_State* L)
{
    auto* self = checkInstance(L, 1);
    auto* parent = lua_isnoneornil(L, 2) ? nullptr : checkInstance(L, 2);
    self->SetParent(parent ? parent->shared_from_this() : nullptr);
    return 0;
}

int instanceAddChild(lua_State* L)
{
    auto* self = checkInstance(L, 1);
    auto* child = checkInstance(L, 2);
    child->SetParent(self->shared_from_this());
    return 0;
}

int instanceFindFirstChild(lua_State* L)
{
    auto* self = checkInstance(L, 1);
    const char* name = luaL_checkstring(L, 2);

    if (auto child = self->FindFirstChild(name))
        pushInstance(L, child.get());
    else
        lua_pushnil(L);

    return 1;
}

int instanceGetChildren(lua_State* L)
{
    auto* self = checkInstance(L, 1);
    const auto children = self->GetChildren();
    lua_createtable(L, static_cast<int>(children.size()), 0);

    int index = 1;
    for (const auto& child : children)
    {
        pushInstance(L, child.get());
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

int instanceGetClass(lua_State* L)
{
    const std::string className = checkInstance(L, 1)->GetClass();
    lua_pushstring(L, className.c_str());
    return 1;
}

int eventFireServer(lua_State* L)
{
    auto* event = checkHorizonEvent(L, 1);
    event->FireServer(readStringArgs(L, 2));
    return 0;
}

int eventFireClient(lua_State* L)
{
    auto* event = checkHorizonEvent(L, 1);
    event->FireClient(readStringArgs(L, 2));
    return 0;
}

int eventFireAllClients(lua_State* L)
{
    auto* event = checkHorizonEvent(L, 1);
    event->FireAllClients(readStringArgs(L, 2));
    return 0;
}

int eventOnServerEvent(lua_State* L)
{
    auto* event = checkHorizonEvent(L, 1);
    const int ref = retainLuaFunction(L, 2);
    event->OnServerEvent([L, ref](const std::vector<std::string>& args) {
        callLuaCallback(L, ref, args);
    });
    return 0;
}

int eventOnClientEvent(lua_State* L)
{
    auto* event = checkHorizonEvent(L, 1);
    const int ref = retainLuaFunction(L, 2);
    event->OnClientEvent([L, ref](const std::vector<std::string>& args) {
        callLuaCallback(L, ref, args);
    });
    return 0;
}

int functionInvokeServer(lua_State* L)
{
    auto* func = checkHorizonFunction(L, 1);
    lua_pushstring(L, func->InvokeServer(readStringArgs(L, 2)).c_str());
    return 1;
}

int functionOnServerInvoke(lua_State* L)
{
    auto* func = checkHorizonFunction(L, 1);
    const int ref = retainLuaFunction(L, 2);
    func->OnServerInvoke([L, ref](const std::vector<std::string>& args) {
        return callLuaHandler(L, ref, args);
    });
    return 0;
}

int bindableFire(lua_State* L)
{
    auto* event = checkBindableEvent(L, 1);
    event->Fire(readStringArgs(L, 2));
    return 0;
}

int bindableConnect(lua_State* L)
{
    auto* event = checkBindableEvent(L, 1);
    const int ref = retainLuaFunction(L, 2);
    event->Connect([L, ref](const std::vector<std::string>& args) {
        callLuaCallback(L, ref, args);
    });
    return 0;
}

int bindableDisconnectAll(lua_State* L)
{
    checkBindableEvent(L, 1)->DisconnectAll();
    return 0;
}

int instanceIndex(lua_State* L)
{
    auto* self = checkInstance(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (std::strcmp(key, "Name") == 0)
        lua_pushstring(L, self->Name.c_str());
    else if (std::strcmp(key, "Parent") == 0 && self->Parent)
        pushInstance(L, self->Parent.get());
    else if (std::strcmp(key, "Parent") == 0)
        lua_pushnil(L);
    else if (std::strcmp(key, "Children") == 0)
        return instanceGetChildren(L);
    else if (std::strcmp(key, "SetParent") == 0)
        lua_pushcfunction(L, instanceSetParent, "Instance.SetParent");
    else if (std::strcmp(key, "AddChild") == 0)
        lua_pushcfunction(L, instanceAddChild, "Instance.AddChild");
    else if (std::strcmp(key, "FindFirstChild") == 0)
        lua_pushcfunction(L, instanceFindFirstChild, "Instance.FindFirstChild");
    else if (std::strcmp(key, "GetChildren") == 0)
        lua_pushcfunction(L, instanceGetChildren, "Instance.GetChildren");
    else if (std::strcmp(key, "GetClass") == 0)
        lua_pushcfunction(L, instanceGetClass, "Instance.GetClass");
    else if (dynamic_cast<Horizon::HorizonEvent*>(self) && std::strcmp(key, "FireServer") == 0)
        lua_pushcfunction(L, eventFireServer, "HorizonEvent.FireServer");
    else if (dynamic_cast<Horizon::HorizonEvent*>(self) && std::strcmp(key, "FireClient") == 0)
        lua_pushcfunction(L, eventFireClient, "HorizonEvent.FireClient");
    else if (dynamic_cast<Horizon::HorizonEvent*>(self) && std::strcmp(key, "FireAllClients") == 0)
        lua_pushcfunction(L, eventFireAllClients, "HorizonEvent.FireAllClients");
    else if (dynamic_cast<Horizon::HorizonEvent*>(self) && std::strcmp(key, "OnServerEvent") == 0)
        pushCallbackMethod(L, eventOnServerEvent, "HorizonEvent.OnServerEvent");
    else if (dynamic_cast<Horizon::HorizonEvent*>(self) && std::strcmp(key, "OnClientEvent") == 0)
        pushCallbackMethod(L, eventOnClientEvent, "HorizonEvent.OnClientEvent");
    else if (dynamic_cast<Horizon::HorizonFunction*>(self) && std::strcmp(key, "InvokeServer") == 0)
        lua_pushcfunction(L, functionInvokeServer, "HorizonFunction.InvokeServer");
    else if (dynamic_cast<Horizon::HorizonFunction*>(self) && std::strcmp(key, "OnServerInvoke") == 0)
        pushCallbackMethod(L, functionOnServerInvoke, "HorizonFunction.OnServerInvoke");
    else if (dynamic_cast<Horizon::BindableEvent*>(self) && std::strcmp(key, "Fire") == 0)
        lua_pushcfunction(L, bindableFire, "BindableEvent.Fire");
    else if (dynamic_cast<Horizon::BindableEvent*>(self) && std::strcmp(key, "Connect") == 0)
        pushCallbackMethod(L, bindableConnect, "BindableEvent.Connect");
    else if (dynamic_cast<Horizon::BindableEvent*>(self) && std::strcmp(key, "DisconnectAll") == 0)
        lua_pushcfunction(L, bindableDisconnectAll, "BindableEvent.DisconnectAll");
    else
        lua_pushnil(L);

    return 1;
}

int instanceNewIndex(lua_State* L)
{
    auto* self = checkInstance(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (std::strcmp(key, "Name") == 0)
        self->Name = luaL_checkstring(L, 3);

    return 0;
}

int instanceNew(lua_State* L)
{
    auto* store = static_cast<InstanceStore*>(lua_tolightuserdata(L, lua_upvalueindex(1)));
    const char* className = luaL_optstring(L, 1, "Instance");

    std::shared_ptr<Horizon::Instance> instance;
    if (std::strcmp(className, "Part") == 0)
        instance = std::make_shared<Horizon::Part>();
    else if (std::strcmp(className, "ServerScript") == 0)
        instance = std::make_shared<Horizon::ServerScript>();
    else if (std::strcmp(className, "LocalScript") == 0)
        instance = std::make_shared<Horizon::LocalScript>();
    else if (std::strcmp(className, "ModuleScript") == 0)
        instance = std::make_shared<Horizon::ModuleScript>();
    else if (std::strcmp(className, "Folder") == 0)
        instance = std::make_shared<Horizon::Folder>();
    else if (std::strcmp(className, "SpotLight") == 0)
        instance = std::make_shared<Horizon::SpotLight>();
    else if (std::strcmp(className, "HorizonEvent") == 0)
        instance = std::make_shared<Horizon::HorizonEvent>();
    else if (std::strcmp(className, "HorizonFunction") == 0)
        instance = std::make_shared<Horizon::HorizonFunction>();
    else if (std::strcmp(className, "BindableEvent") == 0)
        instance = std::make_shared<Horizon::BindableEvent>();
    else
        instance = std::make_shared<Horizon::Instance>();

    instance->Name = luaL_optstring(L, 1, className);
    store->push_back(instance);
    pushInstance(L, instance.get());
    return 1;
}

int serverScriptsRegisterScript(lua_State* L)
{
    checkServerScripts(L, 1)->RegisterScript(luaL_checkstring(L, 2));
    return 0;
}

int serverScriptsGetScripts(lua_State* L)
{
    const auto& scripts = checkServerScripts(L, 1)->GetScripts();
    lua_createtable(L, static_cast<int>(scripts.size()), 0);

    int index = 1;
    for (const auto& script : scripts)
    {
        lua_pushstring(L, script.c_str());
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

int serverScriptsIndex(lua_State* L)
{
    const char* key = luaL_checkstring(L, 2);

    if (std::strcmp(key, "RegisterScript") == 0)
        lua_pushcfunction(L, serverScriptsRegisterScript, "ServerScripts.RegisterScript");
    else if (std::strcmp(key, "GetScripts") == 0)
        lua_pushcfunction(L, serverScriptsGetScripts, "ServerScripts.GetScripts");
    else
        lua_pushnil(L);

    return 1;
}

int loopBindToHeartbeat(lua_State* L)
{
    auto* service = checkLoopService(L, 1);
    const std::string name = luaL_checkstring(L, 2);
    const int ref = retainLuaFunction(L, 3);
    service->BindToHeartbeat(name, [L, ref](float dt) {
        callLuaNumberCallback(L, ref, dt);
    });
    return 0;
}

int loopBindToStepped(lua_State* L)
{
    auto* service = checkLoopService(L, 1);
    const std::string name = luaL_checkstring(L, 2);
    const int ref = retainLuaFunction(L, 3);
    service->BindToStepped(name, [L, ref](float dt) {
        callLuaNumberCallback(L, ref, dt);
    });
    return 0;
}

int loopUnbindFromHeartbeat(lua_State* L)
{
    checkLoopService(L, 1)->UnbindFromHeartbeat(luaL_checkstring(L, 2));
    return 0;
}

int loopUnbindFromStepped(lua_State* L)
{
    checkLoopService(L, 1)->UnbindFromStepped(luaL_checkstring(L, 2));
    return 0;
}

int loopServiceIndex(lua_State* L)
{
    const char* key = luaL_checkstring(L, 2);

    if (std::strcmp(key, "BindToHeartbeat") == 0)
        pushCallbackMethod(L, loopBindToHeartbeat, "LoopService.BindToHeartbeat");
    else if (std::strcmp(key, "BindToStepped") == 0)
        pushCallbackMethod(L, loopBindToStepped, "LoopService.BindToStepped");
    else if (std::strcmp(key, "UnbindFromHeartbeat") == 0)
        lua_pushcfunction(L, loopUnbindFromHeartbeat, "LoopService.UnbindFromHeartbeat");
    else if (std::strcmp(key, "UnbindFromStepped") == 0)
        lua_pushcfunction(L, loopUnbindFromStepped, "LoopService.UnbindFromStepped");
    else
        lua_pushnil(L);

    return 1;
}

int storeSetData(lua_State* L)
{
    checkHorizonStore(L, 1)->SetData(luaL_checkstring(L, 2), luaL_checkstring(L, 3));
    return 0;
}

int storeGetData(lua_State* L)
{
    const auto value = checkHorizonStore(L, 1)->GetData(luaL_checkstring(L, 2));
    lua_pushstring(L, value.c_str());
    return 1;
}

int storeHasKey(lua_State* L)
{
    lua_pushboolean(L, checkHorizonStore(L, 1)->HasKey(luaL_checkstring(L, 2)));
    return 1;
}

int storeRemoveKey(lua_State* L)
{
    checkHorizonStore(L, 1)->RemoveKey(luaL_checkstring(L, 2));
    return 0;
}

int horizonStoreIndex(lua_State* L)
{
    const char* key = luaL_checkstring(L, 2);

    if (std::strcmp(key, "SetData") == 0)
        lua_pushcfunction(L, storeSetData, "HorizonStore.SetData");
    else if (std::strcmp(key, "GetData") == 0)
        lua_pushcfunction(L, storeGetData, "HorizonStore.GetData");
    else if (std::strcmp(key, "HasKey") == 0)
        lua_pushcfunction(L, storeHasKey, "HorizonStore.HasKey");
    else if (std::strcmp(key, "RemoveKey") == 0)
        lua_pushcfunction(L, storeRemoveKey, "HorizonStore.RemoveKey");
    else
        lua_pushnil(L);

    return 1;
}

int messagingPublishAsync(lua_State* L)
{
    checkMessagingService(L, 1)->PublishAsync(luaL_checkstring(L, 2), luaL_checkstring(L, 3));
    return 0;
}

int messagingSubscribeAsync(lua_State* L)
{
    auto* service = checkMessagingService(L, 1);
    const std::string topic = luaL_checkstring(L, 2);
    const int ref = retainLuaFunction(L, 3);
    service->SubscribeAsync(topic, [L, ref](const std::string& message) {
        callLuaStringCallback(L, ref, message);
    });
    return 0;
}

int messagingUnsubscribe(lua_State* L)
{
    checkMessagingService(L, 1)->Unsubscribe(luaL_checkstring(L, 2));
    return 0;
}

int messagingServiceIndex(lua_State* L)
{
    const char* key = luaL_checkstring(L, 2);

    if (std::strcmp(key, "PublishAsync") == 0)
        lua_pushcfunction(L, messagingPublishAsync, "MessagingService.PublishAsync");
    else if (std::strcmp(key, "SubscribeAsync") == 0)
        pushCallbackMethod(L, messagingSubscribeAsync, "MessagingService.SubscribeAsync");
    else if (std::strcmp(key, "Unsubscribe") == 0)
        lua_pushcfunction(L, messagingUnsubscribe, "MessagingService.Unsubscribe");
    else
        lua_pushnil(L);

    return 1;
}

int tweenGc(lua_State* L)
{
    checkTweenHandle(L, 1)->~TweenHandle();
    return 0;
}

int tweenPlay(lua_State* L)
{
    checkTween(L, 1)->Play();
    return 0;
}

int tweenPause(lua_State* L)
{
    checkTween(L, 1)->Pause();
    return 0;
}

int tweenCancel(lua_State* L)
{
    checkTween(L, 1)->Cancel();
    return 0;
}

int tweenIsPlaying(lua_State* L)
{
    lua_pushboolean(L, checkTween(L, 1)->IsPlaying());
    return 1;
}

int tweenIndex(lua_State* L)
{
    const char* key = luaL_checkstring(L, 2);
    if (std::strcmp(key, "Play") == 0)
        lua_pushcfunction(L, tweenPlay, "Tween.Play");
    else if (std::strcmp(key, "Pause") == 0)
        lua_pushcfunction(L, tweenPause, "Tween.Pause");
    else if (std::strcmp(key, "Cancel") == 0)
        lua_pushcfunction(L, tweenCancel, "Tween.Cancel");
    else if (std::strcmp(key, "IsPlaying") == 0)
        lua_pushcfunction(L, tweenIsPlaying, "Tween.IsPlaying");
    else
        lua_pushnil(L);
    return 1;
}

int tweenServiceCreate(lua_State* L)
{
    auto* service = checkTweenService(L, 1);
    auto tween = service->Create(sharedInstance(L, 2), readTweenInfo(L, 3), readFloatGoals(L, 4));
    pushTween(L, std::move(tween));
    return 1;
}

int tweenServiceIndex(lua_State* L)
{
    const char* key = luaL_checkstring(L, 2);
    if (std::strcmp(key, "Create") == 0)
        lua_pushcfunction(L, tweenServiceCreate, "TweenService.Create");
    else
        lua_pushnil(L);
    return 1;
}

int soundInit(lua_State* L)
{
    lua_pushboolean(L, checkSoundService(L, 1)->Init());
    return 1;
}

int soundPlaySound(lua_State* L)
{
    checkSoundService(L, 1)->PlaySound(luaL_checkstring(L, 2));
    return 0;
}

int soundStopAll(lua_State* L)
{
    checkSoundService(L, 1)->StopAll();
    return 0;
}

int soundSetVolume(lua_State* L)
{
    checkSoundService(L, 1)->SetVolume(static_cast<float>(luaL_checknumber(L, 2)));
    return 0;
}

int soundServiceIndex(lua_State* L)
{
    const char* key = luaL_checkstring(L, 2);
    if (std::strcmp(key, "Init") == 0)
        lua_pushcfunction(L, soundInit, "SoundService.Init");
    else if (std::strcmp(key, "PlaySound") == 0)
        lua_pushcfunction(L, soundPlaySound, "SoundService.PlaySound");
    else if (std::strcmp(key, "StopAll") == 0)
        lua_pushcfunction(L, soundStopAll, "SoundService.StopAll");
    else if (std::strcmp(key, "SetVolume") == 0)
        lua_pushcfunction(L, soundSetVolume, "SoundService.SetVolume");
    else
        lua_pushnil(L);
    return 1;
}

int tagAddTag(lua_State* L)
{
    checkTagService(L, 1)->AddTag(sharedInstance(L, 2), luaL_checkstring(L, 3));
    return 0;
}

int tagRemoveTag(lua_State* L)
{
    checkTagService(L, 1)->RemoveTag(sharedInstance(L, 2), luaL_checkstring(L, 3));
    return 0;
}

int tagHasTag(lua_State* L)
{
    lua_pushboolean(L, checkTagService(L, 1)->HasTag(sharedInstance(L, 2), luaL_checkstring(L, 3)));
    return 1;
}

int tagGetTagged(lua_State* L)
{
    const auto tagged = checkTagService(L, 1)->GetTagged(luaL_checkstring(L, 2));
    lua_createtable(L, static_cast<int>(tagged.size()), 0);
    int index = 1;
    for (const auto& instance : tagged)
    {
        if (instance)
        {
            pushInstance(L, instance.get());
            lua_rawseti(L, -2, index++);
        }
    }
    return 1;
}

int tagServiceIndex(lua_State* L)
{
    const char* key = luaL_checkstring(L, 2);
    if (std::strcmp(key, "AddTag") == 0)
        lua_pushcfunction(L, tagAddTag, "TagService.AddTag");
    else if (std::strcmp(key, "RemoveTag") == 0)
        lua_pushcfunction(L, tagRemoveTag, "TagService.RemoveTag");
    else if (std::strcmp(key, "HasTag") == 0)
        lua_pushcfunction(L, tagHasTag, "TagService.HasTag");
    else if (std::strcmp(key, "GetTagged") == 0)
        lua_pushcfunction(L, tagGetTagged, "TagService.GetTagged");
    else
        lua_pushnil(L);
    return 1;
}

void registerInstanceFactory(lua_State* L, InstanceStore* store)
{
    lua_createtable(L, 0, 1);
    lua_pushlightuserdata(L, store);
    lua_pushcclosurek(L, instanceNew, "Instance.new", 1, nullptr);
    lua_setfield(L, -2, "new");
    lua_setglobal(L, "Instance");
}

void registerServiceGlobals(lua_State* L)
{
    pushInstance(L, Horizon::Scene::Get().GetRoot().get());
    lua_setglobal(L, "Scene");

    pushInstance(L, Horizon::SharedStorage::Get().GetRoot().get());
    lua_setglobal(L, "SharedStorage");

    pushServerScripts(L, &Horizon::ServerScripts::Get());
    lua_setglobal(L, "ServerScripts");

    pushLoopService(L, &Horizon::LoopService::Get());
    lua_setglobal(L, "LoopService");

    pushHorizonStore(L, &Horizon::HorizonStore::Get());
    lua_setglobal(L, "HorizonStore");

    pushMessagingService(L, &Horizon::MessagingService::Get());
    lua_setglobal(L, "MessagingService");

    pushTweenService(L, &Horizon::TweenService::Get());
    lua_setglobal(L, "TweenService");

    pushSoundService(L, &Horizon::SoundService::Get());
    lua_setglobal(L, "SoundService");

    pushTagService(L, &Horizon::TagService::Get());
    lua_setglobal(L, "TagService");
}

void registerInstanceBindings(lua_State* L, InstanceStore* store, CallbackRefStore* refs)
{
    if (luaL_newmetatable(L, kInstanceMetatable))
    {
        lua_pushlightuserdata(L, refs);
        lua_pushcclosurek(L, instanceIndex, "Instance.__index", 1, nullptr);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, instanceNewIndex, "Instance.__newindex");
        lua_setfield(L, -2, "__newindex");
    }

    lua_pop(L, 1);

    if (luaL_newmetatable(L, kServerScriptsMetatable))
    {
        lua_pushcfunction(L, serverScriptsIndex, "ServerScripts.__index");
        lua_setfield(L, -2, "__index");
    }

    lua_pop(L, 1);

    if (luaL_newmetatable(L, kLoopServiceMetatable))
    {
        lua_pushlightuserdata(L, refs);
        lua_pushcclosurek(L, loopServiceIndex, "LoopService.__index", 1, nullptr);
        lua_setfield(L, -2, "__index");
    }

    lua_pop(L, 1);

    if (luaL_newmetatable(L, kHorizonStoreMetatable))
    {
        lua_pushcfunction(L, horizonStoreIndex, "HorizonStore.__index");
        lua_setfield(L, -2, "__index");
    }

    lua_pop(L, 1);

    if (luaL_newmetatable(L, kMessagingServiceMetatable))
    {
        lua_pushlightuserdata(L, refs);
        lua_pushcclosurek(L, messagingServiceIndex, "MessagingService.__index", 1, nullptr);
        lua_setfield(L, -2, "__index");
    }

    lua_pop(L, 1);

    if (luaL_newmetatable(L, kTweenMetatable))
    {
        lua_pushcfunction(L, tweenIndex, "Tween.__index");
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, tweenGc, "Tween.__gc");
        lua_setfield(L, -2, "__gc");
    }

    lua_pop(L, 1);

    if (luaL_newmetatable(L, kTweenServiceMetatable))
    {
        lua_pushcfunction(L, tweenServiceIndex, "TweenService.__index");
        lua_setfield(L, -2, "__index");
    }

    lua_pop(L, 1);

    if (luaL_newmetatable(L, kSoundServiceMetatable))
    {
        lua_pushcfunction(L, soundServiceIndex, "SoundService.__index");
        lua_setfield(L, -2, "__index");
    }

    lua_pop(L, 1);

    if (luaL_newmetatable(L, kTagServiceMetatable))
    {
        lua_pushcfunction(L, tagServiceIndex, "TagService.__index");
        lua_setfield(L, -2, "__index");
    }

    lua_pop(L, 1);
    registerInstanceFactory(L, store);
    registerServiceGlobals(L);
}

} // namespace

namespace Horizon::Scripting {

struct LuauVM::Impl {
    lua_State* state = nullptr;
    bool loaded = false;
    InstanceStore instances;
    CallbackRefStore callbackRefs;

    ~Impl()
    {
        for (const auto& instance : instances)
        {
            instance->Parent.reset();
            instance->Children.clear();
        }
    }
};

LuauVM::LuauVM()
    : impl_(std::make_unique<Impl>())
{
    impl_->state = luaL_newstate();
    if (impl_->state)
    {
        luaL_openlibs(impl_->state);
        registerPrintHandler(impl_->state);
        registerInstanceBindings(impl_->state, &impl_->instances, &impl_->callbackRefs);

        if (loadScript(kServicesSmokeTest))
            execute();
    }
}

LuauVM::~LuauVM()
{
    if (impl_ && impl_->state)
    {
        for (const int ref : impl_->callbackRefs)
            lua_unref(impl_->state, ref);

        impl_->callbackRefs.clear();
        lua_close(impl_->state);
        impl_->state = nullptr;
    }
}

LuauVM::LuauVM(LuauVM&&) noexcept = default;

LuauVM& LuauVM::operator=(LuauVM&&) noexcept = default;

void LuauVM::SetEditorUI(Horizon::EditorUI* ui)
{
    editorRef = ui;
}

bool LuauVM::loadScript(const std::string& source)
{
    if (!impl_->state)
        return false;

    const std::string bytecode = Luau::compile(source);
    const int status = luau_load(
        impl_->state,
        "HorizonScript",
        bytecode.data(),
        bytecode.size(),
        0);

    if (status != LUA_OK)
    {
        const char* message = lua_tostring(impl_->state, -1);
        std::cerr << "Luau load failed: " << (message ? message : "unknown error") << std::endl;
        lua_pop(impl_->state, 1);
        impl_->loaded = false;
        return false;
    }

    impl_->loaded = true;
    return true;
}

bool LuauVM::execute()
{
    if (!impl_->state || !impl_->loaded)
        return false;

    const int status = lua_pcall(impl_->state, 0, 0, 0);
    impl_->loaded = false;

    if (status != LUA_OK)
    {
        const char* message = lua_tostring(impl_->state, -1);
        std::cerr << "Luau execute failed: " << (message ? message : "unknown error") << std::endl;
        lua_pop(impl_->state, 1);
        return false;
    }

    std::fflush(stdout);
    return true;
}

} // namespace Horizon::Scripting
