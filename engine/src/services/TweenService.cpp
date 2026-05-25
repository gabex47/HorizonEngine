#include "TweenService.h"

#include "../Part.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

constexpr float kPi = 3.14159265358979323846f;

float clamp01(float value) { return std::clamp(value, 0.0f, 1.0f); }

bool readPartProperty(Horizon::Part* part, const std::string& name, float& value)
{
    if (!part)
        return false;
    if (name == "PositionX") value = part->Position.X; else if (name == "PositionY") value = part->Position.Y;
    else if (name == "PositionZ") value = part->Position.Z; else if (name == "SizeX") value = part->Size.X;
    else if (name == "SizeY") value = part->Size.Y; else if (name == "SizeZ") value = part->Size.Z; else return false;
    return true;
}

void writePartProperty(Horizon::Part* part, const std::string& name, float value)
{
    if (name == "PositionX") part->Position.X = value; else if (name == "PositionY") part->Position.Y = value;
    else if (name == "PositionZ") part->Position.Z = value; else if (name == "SizeX") part->Size.X = value;
    else if (name == "SizeY") part->Size.Y = value; else if (name == "SizeZ") part->Size.Z = value;
}

} // namespace

namespace Horizon {

Tween::Tween(std::shared_ptr<Instance> instance, TweenInfo info, std::unordered_map<std::string, float> goals)
    : instance_(std::move(instance)), info_(std::move(info)), goals_(std::move(goals)) {}

void Tween::Play()
{
    if (finished_)
    {
        elapsed_ = 0.0f;
        repeatsDone_ = 0;
        reversed_ = false;
        finished_ = false;
        starts_.clear();
    }
    if (starts_.empty())
        CaptureStarts();
    playing_ = true;
}

void Tween::Pause() { playing_ = false; }

void Tween::Cancel() { playing_ = false; finished_ = true; }

bool Tween::IsPlaying() { return playing_; }

bool Tween::IsFinished() const { return finished_; }

void Tween::Tick(float deltaTime)
{
    if (!playing_ || finished_ || !instance_)
        return;

    const float duration = std::max(info_.Duration, 0.0001f);
    elapsed_ += std::max(deltaTime, 0.0f);
    const float alpha = clamp01(elapsed_ / duration);
    Apply(reversed_ ? 1.0f - Ease(alpha) : Ease(alpha));

    if (elapsed_ < duration) return;

    if (repeatsDone_ < info_.RepeatCount)
    {
        ++repeatsDone_;
        elapsed_ = 0.0f;
        if (info_.Reverses) reversed_ = !reversed_;
        return;
    }

    playing_ = false;
    finished_ = true;
}

void Tween::CaptureStarts()
{
    auto* part = dynamic_cast<Part*>(instance_.get());
    for (const auto& goal : goals_)
    {
        float start = 0.0f;
        if (readPartProperty(part, goal.first, start))
            starts_[goal.first] = start;
    }
}

void Tween::Apply(float alpha)
{
    auto* part = dynamic_cast<Part*>(instance_.get());
    if (!part)
        return;

    for (const auto& start : starts_)
    {
        const float goal = goals_.at(start.first);
        writePartProperty(part, start.first, start.second + (goal - start.second) * alpha);
    }
}

float Tween::Ease(float t) const
{
    t = clamp01(t);
    auto out = [this](float value) {
        if (info_.EasingStyle == "Quad")
            return 1.0f - (1.0f - value) * (1.0f - value);
        if (info_.EasingStyle == "Sine")
            return std::sin(value * kPi * 0.5f);
        return value;
    };

    if (info_.EasingDirection == "In")
        return 1.0f - out(1.0f - t);
    if (info_.EasingDirection == "InOut")
        return t < 0.5f ? (1.0f - out(1.0f - 2.0f * t)) * 0.5f : 0.5f + out(2.0f * t - 1.0f) * 0.5f;
    return out(t);
}

TweenService& TweenService::Get() { static TweenService service; return service; }

std::shared_ptr<Tween> TweenService::Create(std::shared_ptr<Instance> instance, TweenInfo info, std::unordered_map<std::string, float> goals)
{
    auto tween = std::make_shared<Tween>(std::move(instance), std::move(info), std::move(goals));
    tweens_.push_back(tween);
    return tween;
}

void TweenService::Tick(float deltaTime)
{
    const auto tweens = tweens_;
    for (const auto& tween : tweens)
        if (tween)
            tween->Tick(deltaTime);

    tweens_.erase(std::remove_if(tweens_.begin(), tweens_.end(),
                      [](const auto& tween) { return !tween || tween->IsFinished(); }),
        tweens_.end());
}

} // namespace Horizon
