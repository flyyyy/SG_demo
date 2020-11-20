#include "ErEnvmap.h"

ErEnvmap::ErEnvmap(const GLchar *erMapPath) :
    envShader("./shader/envmap.vs", "./shader/envmap.fs")
{
    // load equirectangular map
    GLuint erMap;
    glGenTextures(1, &erMap);
    glBindTexture(GL_TEXTURE_2D, erMap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLint width, height, channel;
    stbi_set_flip_vertically_on_load(true);
    GLubyte *data = stbi_load(erMapPath, &width, &height, &channel, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    // generate FBO & RBO
    GLuint FBO, RBO;
    glGenFramebuffers(1, &FBO);
    glGenRenderbuffers(1, &RBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBO);
    // generate envmap
    glGenTextures(1, &envmap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envmap);
    for (GLuint i = 0; i < 6; i++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // projection & view matrices
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 views[] = {
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };
    // render to cubemap
    Shader er2cubemapShader("./shader/er2cubemap.vs", "./shader/er2cubemap.fs");
    er2cubemapShader.use();
    er2cubemapShader.setInt("erMap", 0);
    er2cubemapShader.setMat4("projection", projection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, erMap);
    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    for (GLuint i = 0; i < 6; i++)
    {
        er2cubemapShader.setMat4("view", views[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envmap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // generate diffuse cubemap
    glGenTextures(1, &diffuseCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, diffuseCubemap);
    for (GLuint i = 0; i < 6; i++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // bind frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
    // render to diffuse cubemap
    Shader diffuseCubemapShader("./shader/diffusecubemap.vs", "./shader/diffusecubemap.fs");
    diffuseCubemapShader.use();
    diffuseCubemapShader.setInt("envmap", 0);
    diffuseCubemapShader.setMat4("projection", projection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envmap);
    glViewport(0, 0, 32, 32);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    for (GLuint i = 0; i < 6; i++)
    {
        diffuseCubemapShader.setMat4("view", views[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, diffuseCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // generate prefilter cubemap
    glGenTextures(1, &prefilterCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterCubemap);
    for (GLuint i = 0; i < 6; i++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    // render to prefilter cubemap
    Shader prefilterShader("./shader/prefiltercubemap.vs", "./shader/prefiltercubemap.fs");
    prefilterShader.use();
    prefilterShader.setInt("envmap", 0);
    prefilterShader.setMat4("projection", projection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envmap);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    for (GLuint mip = 0; mip < 5; mip++)
    {
        GLuint mipWidth = 128 * std::pow(0.5, mip);
        GLuint mipHeight = 128 * std::pow(0.5, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);
        float roughness = 1.0f * mip / 4;
        prefilterShader.setFloat("roughness", roughness);
        for (GLuint i = 0; i < 6; i++)
        {
            prefilterShader.setMat4("view", views[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterCubemap, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            render();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // generate BRDF map
    glGenTextures(1, &brdfMap);
    glBindTexture(GL_TEXTURE_2D, brdfMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // render to BRDF map
    Shader brdfShader("./shader/brdfMap.vs", "./shader/brdfMap.fs");
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfMap, 0);
    glViewport(0, 0, 512, 512);
    brdfShader.use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // release
    glDeleteTextures(1, &erMap);
    glDeleteFramebuffers(1, &FBO);
    glDeleteRenderbuffers(1, &RBO);
}

ErEnvmap::~ErEnvmap()
{
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteTextures(1, &envmap);
    glDeleteTextures(1, &diffuseCubemap);
    glDeleteTextures(1, &prefilterCubemap);
    glDeleteTextures(1, &brdfMap);
}

void ErEnvmap::renderQuad()
{
    if (!quadVAO)
    {
        GLfloat vertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void ErEnvmap::render()
{
    if (!cubeVAO)
    {
        // 1x1x1 cube data
        GLfloat vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f, // bottom-left
             1.0f,  1.0f, -1.0f, // top-right
             1.0f, -1.0f, -1.0f, // bottom-right
             1.0f,  1.0f, -1.0f, // top-right
            -1.0f, -1.0f, -1.0f, // bottom-left
            -1.0f,  1.0f, -1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f, // bottom-left
             1.0f, -1.0f,  1.0f, // bottom-right
             1.0f,  1.0f,  1.0f, // top-right
             1.0f,  1.0f,  1.0f, // top-right
            -1.0f,  1.0f,  1.0f, // top-left
            -1.0f, -1.0f,  1.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, // top-right
            -1.0f,  1.0f, -1.0f, // top-left
            -1.0f, -1.0f, -1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f, // top-left
             1.0f, -1.0f, -1.0f, // bottom-right
             1.0f,  1.0f, -1.0f, // top-right
             1.0f, -1.0f, -1.0f, // bottom-right
             1.0f,  1.0f,  1.0f, // top-left
             1.0f, -1.0f,  1.0f, // bottom-left
            // bottom face
            -1.0f, -1.0f, -1.0f, // top-right
             1.0f, -1.0f, -1.0f, // top-left
             1.0f, -1.0f,  1.0f, // bottom-left
             1.0f, -1.0f,  1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, // bottom-right
            -1.0f, -1.0f, -1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f, // top-left
             1.0f,  1.0f , 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f, // top-right
             1.0f,  1.0f,  1.0f, // bottom-right
            -1.0f,  1.0f, -1.0f, // top-left
            -1.0f,  1.0f,  1.0f  // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void ErEnvmap::render(const glm::mat4 &view, const glm::mat4 &projection)
{
    // use env shader
    envShader.use();
    envShader.setInt("envmap", 0);
    envShader.setMat4("view", view);
    envShader.setMat4("projection", projection);
    // render
    glDepthMask(GL_FALSE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envmap);
    render();
    glDepthMask(GL_TRUE);
}

void ErEnvmap::activePBR()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, diffuseCubemap);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterCubemap);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, brdfMap);
}
