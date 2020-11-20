#ifndef MODEL_H
#define MODEL_H

#include <map>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Mesh.h"

struct AABB
{
    glm::vec3 minCoords;
    glm::vec3 maxCoords;
};

class Model
{
public:
    Model(std::string path);
    virtual ~Model();

    void draw(Shader& shader);
    AABB getAABB() const;

protected:
    void loadModel(std::string path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh *loadMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Texture> loadTextures(aiMaterial *material, aiTextureType type, std::string typeStr);

private:
    std::vector<Mesh *> meshes;
    std::map<std::string, Texture> loadedTextures;
    std::string dir;
    AABB aabb;
};

#endif // MODEL_H
