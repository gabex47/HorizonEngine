cmake_minimum_required(VERSION 3.24)

project(HorizonEngine VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

find_package(SDL3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.8
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR cmake
)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw glad glm)

add_library(HorizonShared INTERFACE)
target_include_directories(HorizonShared INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/include
)
target_compile_features(HorizonShared INTERFACE cxx_std_20)

add_library(HorizonCore STATIC
    engine/src/Instance.cpp
    engine/src/Part.cpp
    engine/src/events/HorizonEvent.cpp
    engine/src/events/HorizonFunction.cpp
    engine/src/renderer/Camera.cpp
    engine/src/renderer/Renderer.cpp
    engine/src/renderer/Shader.cpp
    engine/src/services/HorizonStore.cpp
    engine/src/services/LoopService.cpp
    engine/src/services/MessagingService.cpp
    engine/src/services/Scene.cpp
    engine/src/services/SharedStorage.cpp
    engine/src/services/ServerScripts.cpp
)
target_include_directories(HorizonCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine/src)
target_compile_features(HorizonCore PUBLIC cxx_std_20)
target_link_libraries(HorizonCore PUBLIC
    HorizonShared
    glfw
    glm::glm
    nlohmann_json::nlohmann_json
    OpenGL::GL
)

add_subdirectory(scripting)

add_executable(HorizonEngine engine/src/main.cpp)
target_compile_features(HorizonEngine PRIVATE cxx_std_20)
target_link_libraries(HorizonEngine PRIVATE
    HorizonShared
    HorizonScripting
    SDL3::SDL3
)

if(MSVC)
    target_compile_options(HorizonEngine PRIVATE /W4 /permissive-)
else()
    target_compile_options(HorizonEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()
