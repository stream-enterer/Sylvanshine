#include "shader.hpp"

#include <GL/gl.h>

#include <iostream>

Shader::Shader() : program_id_(0) {}

Shader::~Shader() {
    Unload();
}

bool Shader::Load(const std::string& vertex_source, const std::string& fragment_source) {
    unsigned int vertex_shader = CompileShader(GL_VERTEX_SHADER, vertex_source);
    if (vertex_shader == 0) return false;

    unsigned int fragment_shader = CompileShader(GL_FRAGMENT_SHADER, fragment_source);
    if (fragment_shader == 0) {
        glDeleteShader(vertex_shader);
        return false;
    }

    program_id_ = glCreateProgram();
    glAttachShader(program_id_, vertex_shader);
    glAttachShader(program_id_, fragment_shader);
    glLinkProgram(program_id_);

    int success;
    glGetProgramiv(program_id_, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(program_id_, 512, nullptr, info_log);
        std::cerr << "Shader program linking failed: " << info_log << std::endl;
        glDeleteProgram(program_id_);
        program_id_ = 0;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program_id_ != 0;
}

void Shader::Use() const {
    glUseProgram(program_id_);
}

void Shader::Unload() {
    if (program_id_ != 0) {
        glDeleteProgram(program_id_);
        program_id_ = 0;
    }
}

void Shader::SetMat4(const std::string& name, const glm::mat4& value) const {
    int location = glGetUniformLocation(program_id_, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value) const {
    int location = glGetUniformLocation(program_id_, name.c_str());
    glUniform4fv(location, 1, &value[0]);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const {
    int location = glGetUniformLocation(program_id_, name.c_str());
    glUniform3fv(location, 1, &value[0]);
}

void Shader::SetFloat(const std::string& name, float value) const {
    int location = glGetUniformLocation(program_id_, name.c_str());
    glUniform1f(location, value);
}

void Shader::SetInt(const std::string& name, int value) const {
    int location = glGetUniformLocation(program_id_, name.c_str());
    glUniform1i(location, value);
}

unsigned int Shader::CompileShader(unsigned int type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        std::cerr << "Shader compilation failed ("
                  << (type == GL_VERTEX_SHADER ? "vertex" : "fragment")
                  << "): " << info_log << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}
