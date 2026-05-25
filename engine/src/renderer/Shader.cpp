#include "Shader.h"

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

namespace Horizon {

Shader::~Shader()
{
    Destroy();
}

void Shader::Destroy()
{
    if (programId != 0)
    {
        glDeleteProgram(programId);
        programId = 0;
    }
}

bool Shader::Load(const char* vertexSource, const char* fragmentSource)
{
    const unsigned int vertex = Compile(GL_VERTEX_SHADER, vertexSource);
    const unsigned int fragment = Compile(GL_FRAGMENT_SHADER, fragmentSource);
    if (vertex == 0 || fragment == 0)
        return false;

    programId = glCreateProgram();
    glAttachShader(programId, vertex);
    glAttachShader(programId, fragment);
    glLinkProgram(programId);

    int success = 0;
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (success)
        return true;

    std::vector<char> log(512);
    glGetProgramInfoLog(programId, static_cast<int>(log.size()), nullptr, log.data());
    std::cerr << "Shader link failed: " << log.data() << std::endl;
    return false;
}

void Shader::Use() const
{
    glUseProgram(programId);
}

void Shader::SetMat4(const std::string& name, const glm::mat4& value) const
{
    glUniformMatrix4fv(glGetUniformLocation(programId, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(glGetUniformLocation(programId, name.c_str()), 1, glm::value_ptr(value));
}

unsigned int Shader::Compile(unsigned int type, const char* source) const
{
    const unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success)
        return shader;

    std::vector<char> log(512);
    glGetShaderInfoLog(shader, static_cast<int>(log.size()), nullptr, log.data());
    std::cerr << "Shader compile failed: " << log.data() << std::endl;
    glDeleteShader(shader);
    return 0;
}

} // namespace Horizon
