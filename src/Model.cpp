#include "Model.h"

Model::Model(std::string path)
{
    aabb.minCoords = glm::vec3(1e9);
    aabb.maxCoords = glm::vec3(-1e9);
    loadModel(path);
}

Model::~Model()
{
    for (size_t i = 0; i < meshes.size(); i++)
        delete meshes[i];
    for (auto o : loadedTextures)
        glDeleteTextures(1, &o.second.id);
    meshes.clear();
    loadedTextures.clear();
}

void Model::draw(Shader& shader)
{
    for (size_t i = 0; i < meshes.size(); i++)
        meshes[i]->draw(shader);
}

void Model::loadModel(std::string path)
{
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals);
    // check
    if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        printf("Load model error: %s\n", importer.GetErrorString());
        return;
    }
    dir = path.substr(0, path.find_last_of('/'));
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
    // puts("!!!");
    for (size_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        // printf("%u\n", mesh);
        meshes.push_back(loadMesh(mesh, scene));
    }
    for (size_t i = 0; i < node->mNumChildren; i++)
        processNode(node->mChildren[i], scene);
}

Mesh *Model::loadMesh(aiMesh *mesh, const aiScene *scene)
{
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;

    // read vertex
    for (size_t i = 0; i < mesh->mNumVertices; i++)
    {
        //printf("vertex %d\n", i);
        Vertex vertex;
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        //puts("miao miao miao");
        aabb.minCoords = glm::min(aabb.minCoords, vertex.position);
        aabb.maxCoords = glm::max(aabb.maxCoords, vertex.position);
        //puts("mie mie mie");
        vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        //puts("wang wang wang");
        if (mesh->mTextureCoords[0])
            vertex.texCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        else
            vertex.texCoords = glm::vec2(0.0f);
        vertices.push_back(vertex);
    }

    // puts("vertex");

    // read face index
    for (size_t i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (size_t j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // puts("face");

    // read materials
    glm::vec3 kD(1.0f), kS(1.0f);
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        // texture
        std::vector<Texture> diffuseTexture = loadTextures(material, aiTextureType_DIFFUSE, "diffuseTexture");
        textures.insert(textures.end(), diffuseTexture.begin(), diffuseTexture.end());
        std::vector<Texture> specularTexture = loadTextures(material, aiTextureType_SPECULAR, "specularTexture");
        textures.insert(textures.end(), specularTexture.begin(), specularTexture.end());

        // constant
        aiColor4D akD, akS;
        if (aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &akD) == aiReturn_SUCCESS)
            kD = glm::vec3(akD[0], akD[1], akD[2]);
        if (aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &akS) == aiReturn_SUCCESS)
            kS = glm::vec3(akS[0], akS[1], akS[2]);
    }

    // puts("material");

    // create mesh
    return new Mesh(vertices, indices, textures, kD, kS);
}

std::vector<Texture> Model::loadTextures(aiMaterial *material, aiTextureType type, std::string typeStr)
{
    std::vector<Texture> textures;
    for (size_t i = 0; i < material->GetTextureCount(type); i++)
    {
        aiString buf;
        material->GetTexture(type, i, &buf);
        std::string path;
        path.assign(buf.C_Str());
        if (loadedTextures.find(path) == loadedTextures.end())
        {
            Texture texture;
            std::string imgPath = dir + "/" + path;
            printf("load texture %s\n", imgPath.c_str());
            glGenTextures(1, &texture.id);
            int width, height, channel;
            stbi_set_flip_vertically_on_load(true);
            GLubyte *data = stbi_load(imgPath.c_str(), &width, &height, &channel, 0);
            printf("w=%d, h=%d, c=%d\n", width, height, channel);
            glBindTexture(GL_TEXTURE_2D, texture.id);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
            stbi_image_free(data);
            texture.type = typeStr;
            textures.push_back(texture);
            loadedTextures[path] = texture;
            puts("load ok!");
        }
        else textures.push_back(loadedTextures[path]);
    }
    return textures;
}

AABB Model::getAABB() const
{
    return aabb;
}
