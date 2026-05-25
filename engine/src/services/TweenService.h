#pragma once

#include "../Instance.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Horizon {

struct TweenInfo {
    float Duration = 1.0f;
    std::string EasingStyle = "Linear";
    std::string EasingDirection = "Out";
    int RepeatCount = 0;
    bool Reverses = false;
};

class Tween {
public:
    Tween(std::shared_ptr<Instance> instance, TweenInfo info, std::unordered_map<std::string, float> goals);

    void Play();
    void Pause();
    void Cancel();
    bool IsPlaying();
    void Tick(float deltaTime);
    bool IsFinished() const;

private:
    void CaptureStarts();
    void Apply(float alpha);
    float Ease(float t) const;

    std::shared_ptr<Instance> instance_;
    TweenInfo info_;
    std::unordered_map<std::string, float> goals_;
    std::unordered_map<std::string, float> starts_;
    float elapsed_ = 0.0f;
    int repeatsDone_ = 0;
    bool playing_ = false;
    bool finished_ = false;
    bool reversed_ = false;
};

class TweenService final {
public:
    std::shared_ptr<Tween> Create(
        std::shared_ptr<Instance> instance,
        TweenInfo info,
        std::unordered_map<std::string, float> goals);
    void Tick(float deltaTime);
    static TweenService& Get();

private:
    TweenService() = default;

    std::vector<std::shared_ptr<Tween>> tweens_;
};

} // namespace Horizon
