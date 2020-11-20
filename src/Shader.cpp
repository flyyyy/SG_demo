#include "Shader.h"

Shader::Shader(const GLchar *vertexShaderPath, const GLchar *fragmentShaderPath)
{
    std::string vertexShaderCode;
    std::string fragmentShaderCode;
    std::ifstream vertexShaderFile;
    std::ifstream fragmentShaderFile;
    vertexShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragmentShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // open shader file
        vertexShaderFile.open(vertexShaderPath);
        fragmentShaderFile.open(fragmentShaderPath);
        std::stringstream vertexShaderStream, fragmentShaderStream;
        // read shader file content
        vertexShaderStream << vertexShaderFile.rdbuf();
        fragmentShaderStream << fragmentShaderFile.rdbuf();
        // close shader file
        vertexShaderFile.close();
        fragmentShaderFile.close();
        // copy
        vertexShaderCode = vertexShaderStream.str();
        fragmentShaderCode = fragmentShaderStream.str();
    }
    catch (std::ifstream::failure e)
    {
        std::cout << "Read Shader File Error." << std::endl << std::endl;
    }
    const GLchar *vertexShaderSource = vertexShaderCode.c_str();
    const GLchar *fragmentShaderSource = fragmentShaderCode.c_str();

    // compile shader
    GLuint vertexShader, fragmentShader;
    GLint success;
    char log[512];
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, log);
        std::cout << "[Compile Vertex Shader Error]\n" << log << std::endl << std::endl;
    }
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, log);
        std::cout << "[Compile Fragment Shader Error]\n" << log << std::endl << std::endl;
    }

    // create shader program
    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cout << "link error " << fragmentShaderPath << std::endl;
        std::cout << "[Link Shader Program Error]\n" << log << std::endl << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader()
{
    glDeleteProgram(program);
}

void Shader::use()
{
    glUseProgram(program);
}

void Shader::setInt(const GLchar *name, GLint value)
{
    glUniform1i(glGetUniformLocation(program, name), value);
}

void Shader::setFloat(const GLchar *name, GLfloat value)
{
    glUniform1f(glGetUniformLocation(program, name), value);
}

void Shader::setVec2(const GLchar *name, const glm::vec2 &value)
{
    glUniform2fv(glGetUniformLocation(program, name), 1, &value[0]);
}

void Shader::setVec3(const GLchar *name, const glm::vec3 &value)
{
    glUniform3fv(glGetUniformLocation(program, name), 1, &value[0]);
}

void Shader::setVec4(const GLchar *name, const glm::vec4 &value)
{
    glUniform4fv(glGetUniformLocation(program, name), 1, &value[0]);
}

void Shader::setMat2(const GLchar *name, const glm::mat2 &value)
{
    glUniformMatrix2fv(glGetUniformLocation(program, name), 1, GL_FALSE, &value[0][0]);
}

void Shader::setMat3(const GLchar *name, const glm::mat3 &value)
{
    glUniformMatrix3fv(glGetUniformLocation(program, name), 1, GL_FALSE, &value[0][0]);
}

void Shader::setMat4(const GLchar *name, const glm::mat4 &value)
{
    glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, &value[0][0]);
}
