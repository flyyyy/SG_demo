#ifndef SHADER_H
#define SHADER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <GL/glew.h>
#include <glm/glm.hpp>

class Shader
{
public:
    Shader(const GLchar *vertexShaderPath, const GLchar *fragmentShaderPath);
    virtual ~Shader();

    void use();

    void setInt(const GLchar *name, GLint value);
    void setFloat(const GLchar *name, GLfloat value);
    void setVec2(const GLchar *name, const glm::vec2 &value);
    void setVec3(const GLchar *name, const glm::vec3 &value);
    void setVec4(const GLchar *name, const glm::vec4 &value);
    void setMat2(const GLchar *name, const glm::mat2 &value);
    void setMat3(const GLchar *name, const glm::mat3 &value);
    void setMat4(const GLchar *name, const glm::mat4 &value);

private:
    GLuint program;
};

#endif // SHADER_H
