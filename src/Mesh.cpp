#include "Mesh.h"

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const std::vector<Texture>& textures, const glm::vec3 kD, const glm::vec3 kS)
{
    this->textures = textures;
    this->indexCount = indices.size();
    this->init(vertices, indices);
    this->kD = kD;
    this->kS = kS;
    puts("[mesh]");
    printf("kD = %f, %f, %f\n", kD[0], kD[1], kD[2]);
    printf("kS = %f, %f, %f\n", kS[0], kS[1], kS[2]);
}

Mesh::~Mesh()
{
    textures.clear();
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}

void Mesh::draw(Shader& shader)
{
    char name[32];
    GLuint diffuse = 0, specular = 0;
    shader.setVec3("kD", kD);
    shader.setVec3("kS", kS);
    shader.setInt("hasTexture", !this->textures.empty());
    for (size_t i = 0; i < textures.size(); i++)
    {
        if (textures[i].type[0] == 'd')
            sprintf(name, "diffuseTexture%d", diffuse++);
        else if (textures[i].type[0] == 's')
            sprintf(name, "specularTexture%d", specular++);
        glActiveTexture(GL_TEXTURE0 + i);
        shader.setInt(name, i);
        glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
    }
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    for (size_t i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Mesh::init(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, texCoords));

    glBindVertexArray(0);
}
