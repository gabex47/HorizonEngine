#include "SoundService.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <system_error>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace Horizon {

struct MiniaudioEngine {
    ma_engine engine;
};

SoundService& SoundService::Get()
{
    static SoundService service;
    return service;
}

SoundService::~SoundService()
{
    StopAll();
    if (initialized_ && engine_)
        ma_engine_uninit(&engine_->engine);
}

bool SoundService::Init()
{
    if (initialized_)
        return true;

    engine_ = std::make_unique<MiniaudioEngine>();
    if (ma_engine_init(nullptr, &engine_->engine) != MA_SUCCESS)
    {
        engine_.reset();
        initialized_ = false;
        return false;
    }

    initialized_ = true;
    ma_engine_set_volume(&engine_->engine, volume_);
    return true;
}

void SoundService::PlaySound(const std::string& filePath)
{
    std::error_code error;
    if (!std::filesystem::exists(filePath, error))
    {
        std::cerr << "[WARN] Sound file not found: " << filePath << std::endl;
        return;
    }

    if (!initialized_ && !Init())
        return;

    ma_engine_play_sound(&engine_->engine, filePath.c_str(), nullptr);
}

void SoundService::StopAll()
{
    if (initialized_ && engine_)
        ma_engine_stop(&engine_->engine);
}

void SoundService::SetVolume(float volume)
{
    volume_ = std::clamp(volume, 0.0f, 1.0f);
    if (initialized_ && engine_)
        ma_engine_set_volume(&engine_->engine, volume_);
}

} // namespace Horizon
