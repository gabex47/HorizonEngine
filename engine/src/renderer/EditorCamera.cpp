#include "EditorCamera.h"

#include "Input.h"

#include <algorithm>
#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

namespace Horizon {

EditorCamera::EditorCamera()
{
    UpdateVectors();
}

void EditorCamera::Update(float deltaTime, Input& input)
{
    speed_ = std::clamp(speed_ + input.GetScroll() * 0.75f, 0.5f, 100.0f);
    if (input.GetMouseButton(MouseButton::Right))
        lookMode_ = true;
    if (input.GetKey(Key::Escape))
        lookMode_ = false;

    input.SetCursorCaptured(lookMode_);
    HandleMouse(input);
    HandleKeyboard(deltaTime, input);
}

void EditorCamera::HandleKeyboard(float deltaTime, const Input& input)
{
    if (!lookMode_)
        return;

    const float multiplier = input.GetKey(Key::LeftShift) || input.GetKey(Key::RightShift) ? 3.0f : 1.0f;
    const float velocity = speed_ * multiplier * std::max(deltaTime, 0.0f);
    glm::vec3 direction(0.0f);

    if (input.GetKey(Key::W)) direction += front_;
    if (input.GetKey(Key::S)) direction -= front_;
    if (input.GetKey(Key::D)) direction += right_;
    if (input.GetKey(Key::A)) direction -= right_;
    if (input.GetKey(Key::E)) direction += worldUp_;
    if (input.GetKey(Key::Q)) direction -= worldUp_;

    if (glm::length(direction) > 0.0f)
        position_ += glm::normalize(direction) * velocity;
}

void EditorCamera::HandleMouse(Input& input)
{
    if (!lookMode_)
        return;

    const glm::vec2 delta = input.GetMouseDelta() * sensitivity_;
    yaw_ += delta.x;
    pitch_ = std::clamp(pitch_ + delta.y, -89.0f, 89.0f);
    UpdateVectors();
}

glm::mat4 EditorCamera::GetViewMatrix() const
{
    return glm::lookAt(position_, position_ + front_, up_);
}

glm::mat4 EditorCamera::GetProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(fov_), aspectRatio, 0.1f, 1000.0f);
}

void EditorCamera::FocusSelected()
{
    // TODO: focus the Scene View camera on the selected object.
}

void EditorCamera::OrbitMode()
{
    // TODO: add scene-object orbit controls.
}

void EditorCamera::Bookmarks()
{
    // TODO: add saved editor camera bookmarks.
}

void EditorCamera::FlyMode()
{
    // TODO: expose fly-mode settings for editor navigation.
}

void EditorCamera::UpdateVectors()
{
    const float yaw = glm::radians(yaw_);
    const float pitch = glm::radians(pitch_);
    front_ = glm::normalize(glm::vec3(
        std::cos(yaw) * std::cos(pitch),
        std::sin(pitch),
        std::sin(yaw) * std::cos(pitch)));
    right_ = glm::normalize(glm::cross(front_, worldUp_));
    up_ = glm::normalize(glm::cross(right_, front_));
}

} // namespace Horizon
