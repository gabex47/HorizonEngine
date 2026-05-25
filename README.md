# HorizonEngine

HorizonEngine is Opal's cross-platform 3D game engine scaffold.

## Layout

```text
engine/      C++20 engine entry point
editor/      Tauri v2 + React + TypeScript editor
scripting/   Luau VM wrapper
shared/      Shared constants and types
```

## Prerequisites

All platforms need CMake 3.24+, a C++20 compiler, SDL3, Rust, Node.js, and npm.
CMake fetches `luau-lang/luau` automatically, or you can pass an existing
checkout with `-DHORIZON_LUAU_SOURCE=/path/to/luau`.

## macOS

```sh
brew install cmake sdl3 node rust
cmake --preset default
cmake --build --preset default
./build/HorizonEngine
```

Run the editor separately:

```sh
cd editor
npm install
npm run tauri dev
```

## Windows

Install Visual Studio 2022 with C++ tools, CMake, Node.js, and Rust. Install
SDL3 with vcpkg:

```powershell
vcpkg install sdl3:x64-windows
cmake --preset default -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build --preset default
.\build\Debug\HorizonEngine.exe
```

Run the editor separately:

```powershell
cd editor
npm install
npm run tauri dev
```

## Linux

On Ubuntu/Debian, install the native build tools, SDL3, and Tauri webview
packages:

```sh
sudo apt install build-essential cmake nodejs npm libsdl3-dev \
  libwebkit2gtk-4.1-dev libayatana-appindicator3-dev librsvg2-dev \
  libssl-dev pkg-config curl
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
cmake --preset default
cmake --build --preset default
./build/HorizonEngine
```

Run the editor separately:

```sh
cd editor
npm install
npm run tauri dev
```
