#pragma once

#include <memory>
#include <string>

namespace Horizon {

struct MiniaudioEngine;

class SoundService {
public:
    bool Init();
    void PlaySound(const std::string& filePath);
    void StopAll();
    void SetVolume(float volume);
    static SoundService& Get();

private:
    SoundService() = default;
    ~SoundService();

    std::unique_ptr<MiniaudioEngine> engine_;
    float volume_ = 1.0f;
    bool initialized_ = false;
};

} // namespace Horizon
