#ifndef MESH_H
#define MESH_H

#include <stddef.h>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <assimp/types.h>
#include "Shader.h"

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

struct Texture
{
    std::string type;
    GLuint id;
};

class Mesh
{
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const std::vector<Texture>& textures, const glm::vec3 kD, const glm::vec3 kS);
    virtual ~Mesh();

    void draw(Shader& shader);

protected:
    void init(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices);

private:
    GLuint VAO, VBO, EBO;
    glm::vec3 kD, kS;
    std::vector<Texture> textures;
    size_t indexCount;
};

#endif // MESH_H
