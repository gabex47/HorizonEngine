#include "LuauVM.h"

#include "Instance.h"
#include "Part.h"
#include "events/HorizonEvent.h"
#include "events/HorizonFunction.h"
#include "services/HorizonStore.h"
#include "services/LoopService.h"
#include "services/MessagingService.h"
#include "services/Scene.h"
#include "services/ServerScripts.h"
#include "services/SharedStorage.h"

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
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr const char* kInstanceMetatable = "Horizon.Instance";
constexpr const char* kServerScriptsMetatable = "Horizon.ServerScripts";
constexpr const char* kLoopServiceMetatable = "Horizon.LoopService";
constexpr const char* kHorizonStoreMetatable = "Horizon.HorizonStore";
constexpr const char* kMessagingServiceMetatable = "Horizon.MessagingService";
constexpr const char* kServicesSmokeTest = R"(
-- LoopService test
LoopService:BindToHeartbeat("TestBind", function(dt)
  -- just needs to be registered, not printed every frame
end)
print("LoopService bound")

-- HorizonStore test
HorizonStore:SetData("PlayerName", "Rian")
print(HorizonStore:GetData("PlayerName"))
HorizonStore:RemoveKey("PlayerName")
print(tostring(HorizonStore:HasKey("PlayerName")))

print("HorizonStore online")

MessagingService:SubscribeAsync("GameStart", function(msg)
  print("Received: " .. msg)
end)
MessagingService:PublishAsync("GameStart", "Game is starting!")
print("MessagingService online")
)";

using InstanceStore = std::vector<std::shared_ptr<Horizon::Instance>>;
using CallbackRefStore = std::vector<int>;

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

void pushInstance(lua_State* L, Horizon::Instance* instance)
{
    auto** userdata = static_cast<Horizon::Instance**>(
        lua_newuserdata(L, sizeof(Horizon::Instance*)));
    *userdata = instance;
    luaL_getmetatable(L, kInstanceMetatable);
    lua_setmetatable(L, -2);
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
    else if (std::strcmp(className, "HorizonEvent") == 0)
        instance = std::make_shared<Horizon::HorizonEvent>();
    else if (std::strcmp(className, "HorizonFunction") == 0)
        instance = std::make_shared<Horizon::HorizonFunction>();
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
