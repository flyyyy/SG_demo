#ifndef ERENVMAP_H
#define ERENVMAP_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include "Shader.h"
#include "stb_image.h"

class ErEnvmap
{
public:
    ErEnvmap(const GLchar *erMapPath);
    virtual ~ErEnvmap();

    void activePBR();

    void render(const glm::mat4 &view, const glm::mat4 &projection);
    void render();
    void renderQuad();

private:
    Shader envShader;
    GLuint cubeVAO = 0u;
    GLuint cubeVBO = 0u;
    GLuint quadVAO = 0u;
    GLuint quadVBO = 0u;
    GLuint envmap;
    GLuint diffuseCubemap;
    GLuint prefilterCubemap;
    GLuint brdfMap;
};

#endif // ENVSPHERE_H
