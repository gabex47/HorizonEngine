include(FetchContent)

set(HORIZON_LUAU_SOURCE "" CACHE PATH "Path to a luau-lang/luau checkout")
set(LUAU_BUILD_CLI OFF CACHE BOOL "Build Luau command-line tools" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "Build Luau tests" FORCE)
set(LUAU_BUILD_WEB OFF CACHE BOOL "Build Luau web target" FORCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Skip dependency updates after initial fetch" FORCE)

if(HORIZON_LUAU_SOURCE)
    add_subdirectory("${HORIZON_LUAU_SOURCE}" "${CMAKE_BINARY_DIR}/luau")
else()
    FetchContent_Declare(
        luau
        GIT_REPOSITORY https://github.com/luau-lang/luau.git
        GIT_TAG 0.722
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(luau)
endif()

add_library(HorizonScripting STATIC src/LuauVM.cpp)
target_include_directories(HorizonScripting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(HorizonScripting PUBLIC cxx_std_20)
target_link_libraries(HorizonScripting PUBLIC
    HorizonCore
    HorizonShared
    Luau.Compiler
    Luau.VM
)

if(MSVC)
    target_compile_options(HorizonScripting PRIVATE /W4 /permissive-)
else()
    target_compile_options(HorizonScripting PRIVATE -Wall -Wextra -Wpedantic)
endif()
