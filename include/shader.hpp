#include <GL/glew.h>
#ifndef SHADER_HPP
#define SHADER_HPP

#include <glm/glm.hpp>

#include <string>

class Shader {
public:
    Shader();
    ~Shader();

    bool Load(const std::string& vertex_source, const std::string& fragment_source);
    void Use() const;
    void Unload();

    void SetMat4(const std::string& name, const glm::mat4& value) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetInt(const std::string& name, int value) const;

    unsigned int GetProgram() const { return program_id_; }
    bool IsValid() const { return program_id_ != 0; }

private:
    unsigned int program_id_;

    unsigned int CompileShader(unsigned int type, const std::string& source);
};

#endif
