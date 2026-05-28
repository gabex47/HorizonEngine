#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <string>

namespace Horizon {

class Shader final {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool Load(const char* vertexSource, const char* fragmentSource);
    void Destroy();
    void Use() const;
    void SetMat4(const std::string& name, const glm::mat4& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetFloat(const std::string& name, float value) const;

private:
    unsigned int programId = 0;
    unsigned int Compile(unsigned int type, const char* source) const;
};

} // namespace Horizon
