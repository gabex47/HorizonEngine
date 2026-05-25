#pragma once

#include <memory>
#include <string>

namespace Horizon::Scripting {

class LuauVM {
public:
    LuauVM();
    ~LuauVM();

    LuauVM(const LuauVM&) = delete;
    LuauVM& operator=(const LuauVM&) = delete;
    LuauVM(LuauVM&&) noexcept;
    LuauVM& operator=(LuauVM&&) noexcept;

    bool loadScript(const std::string& source);
    bool execute();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Horizon::Scripting
