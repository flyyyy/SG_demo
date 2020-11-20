#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <windows.h>

#include <algorithm>
#include <random>
#include <vector>

#include "ErEnvmap.h"
#include "Model.h"
#include "stb_image_write.h"

struct GlobalState {
    glm::mat4 surfaceMat = glm::mat4(1.0f);
    glm::mat3 invView = glm::mat3(1.0f);
    float surfaceY = 0.0f;
    bool isPressed = false;
    bool dispSurface = false;
    int SGNum = 128;
} state;

struct ModelState {
    Model *model;
    glm::mat4 moveToOri = glm::mat4(1.0f);
    glm::mat4 rescale = glm::mat4(1.0f);
    glm::mat4 objMat = glm::mat4(1.0f);
    glm::mat4 objRotMat = glm::mat4(1.0f);
    glm::mat4 lightRela = glm::mat4(1.0f);
    glm::mat4 lightRot = glm::mat4(1.0f);
    glm::vec3 lastRotVec = glm::vec3(0.0f);
    glm::vec3 modelCenter = glm::vec3(0.0f);
    GLuint backSurfaceVBO, backSurfaceVAO;
} modelState[16];

struct SG {
    SG(glm::vec3 w = glm::vec3(1, 1, 1), glm::vec3 u = glm::vec3(1, 0, 0),
       float alpha = 45.4)
        : weight(w), u(u), alpha(alpha) {}
    glm::vec3 weight;
    glm::vec3 u;
    float alpha;
} SG_light[128];

struct SH {
    glm::vec3 weight;
} SH_light[16];

// void combineLight();
void renderQuad();
void keyboardCallback(GLFWwindow *, int, int, int, int);
void cursorPositionCallback(GLFWwindow *, double, double);
void mouseButtonCallback(GLFWwindow *, int, int, int);

glm::vec3 trackBall(double x, double y, double w, double h) {
    glm::vec3 vec;
    vec[0] = 2.0f * x / w - 1.0f;
    vec[1] = 2.0f * (h - y - 1) / h - 1.0f;
    float d = vec[0] * vec[0] + vec[1] * vec[1];
    vec[2] = d > 1.0f ? 0.0f : std::sqrt(1.0f - d);
    return state.invView * vec;
}

void rm(char *s) {
    for (; *s; s++)
        if (*s == '\r' || *s == '\n') *s = '\0';
}

void exportImage(GLFWwindow *window) {
    int w, h;
    char path[100];
    strcpy(path, "./save.jpg");
    glfwGetFramebufferSize(window, &w, &h);
    GLubyte *data = new GLubyte[w * h * 3];
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
    stbi_flip_vertically_on_write(true);
    stbi_write_png(path, w, h, 3, data, w * 3);
    delete[] data;
    puts("export image ok!");
}

void setConfigImg(GLfloat *configData) {
    for (int i = 0; i < 128; i++) {
        configData[i * 3] = 1;
        configData[i * 3 + 1] = 0;
        configData[i * 3 + 2] = 0;
    }
    int off = 128 * 3;
    for (int i = 0; i < 128; i++) {
        configData[off + i * 3] = SG_light[i].u[0];
        configData[off + i * 3 + 1] = SG_light[i].u[1];
        configData[off + i * 3 + 2] = SG_light[i].u[2];
    }
    off = 128 * 6;
    for (int i = 0; i < 128; i++) {
        configData[off + i * 3] = 0;
        configData[off + i * 3 + 1] = 0;
        configData[off + i * 3 + 2] = 0;
    }
}

int main(int argc, char **argv) {
    //  /====================\
    // || read configuration ||
    //  \====================/
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=
    char modelPath[128], backgroundPath[128], depthPath[128], cmd[128],
        save_path[128];
    std::vector<std::string> modelPaths;
    float elevation = 0.0f, fov = 60.0f, matBuf[16];
    float SGScale = 1.0f, SGTrans = 0.0f;
    FILE *file = fopen("./scene_config.txt", "rt");
    while (fgets(cmd, 128, file)) {
        rm(cmd);
        if (!strcmp(cmd, "model:")) {
            int cnt = 0;
            fscanf(file, "%d", &cnt);
            fgetc(file);
            while (cnt--) {
                fgets(modelPath, 128, file);
                rm(modelPath);
                modelPaths.push_back(std::string(modelPath));
            }
        } else if (!strcmp(cmd, "background:")) {
            fgets(backgroundPath, 128, file);
            rm(backgroundPath);
            // strcpy(backgroundPath, "./black.jpg");
        } else if (!strcmp(cmd, "depth:")) {
            fgets(depthPath, 128, file);
            rm(depthPath);
        } else if (!strcmp(cmd, "elevation:")) {
            fscanf(file, "%f", &elevation);
            elevation = glm::radians(elevation);
        } else if (!strcmp(cmd, "fov:")) {
            fscanf(file, "%f", &fov);
            fov = glm::radians(fov);
        } else if (!strcmp(cmd, "SG:")) {
            for (int i = 0; i < state.SGNum; i++) {
                for (int j = 0; j < 3; j++) {
                    fscanf(file, "%f", &SG_light[i].weight[j]);
                }
            }
        } else if (!strcmp(cmd, "alpha:")) {
            for (int i = 0; i < state.SGNum; i++) {
                fscanf(file, "%f", &SG_light[i].alpha);
            }
        } else if (!strcmp(cmd, "center:")) {
            for (int i = 0; i < state.SGNum; i++) {
                for (int j = 0; j < 3; j++)
                    fscanf(file, "%f", &SG_light[i].u[j]);
                // std::swap(light[i].u[0], light[i].u[2]);
            }
        } else if (!strcmp(cmd, "SH:")) {
            for (int i = 0; i < 16; i++) {
                for (int j = 0; j < 3; j++) {
                    fscanf(file, "%f", &SH_light[i].weight[j]);
                    // printf("%f ", SH_light[i].weight[j]);
                }
                // printf("\n");
            }
        } else if (!strcmp(cmd, "objRotMat:")) {
            for (int i = 0; i < 16; i++) fscanf(file, "%f", matBuf + i);
            memcpy(&modelState[0].objRotMat[0][0], matBuf, sizeof(matBuf));
        } else if (!strcmp(cmd, "objMat:")) {
            for (int i = 0; i < 16; i++) fscanf(file, "%f", matBuf + i);
            memcpy(&modelState[0].objMat[0][0], matBuf, sizeof(matBuf));
        } else if (!strcmp(cmd, "sfMat:")) {
            for (int i = 0; i < 16; i++) fscanf(file, "%f", matBuf + i);
            memcpy(&state.surfaceMat[0][0], matBuf, sizeof(matBuf));
        } else if (!strcmp(cmd, "sfY:")) {
            fscanf(file, "%f", &state.surfaceY);
        } else if (!strcmp(cmd, "save_path:")) {
            fgets(save_path, 128, file);
            printf("save_path: %s\n", save_path);
            rm(save_path);
        } else if (!strcmp(cmd, "SG_scale:")) {
            fscanf(file, "%f", &SGScale);
        } else if (!strcmp(cmd, "SG_trans:")) {
            fscanf(file, "%f", &SGTrans);
        }
    }
    fclose(file);
    // SG scale
    for (int i = 0; i < state.SGNum; i++) {
        SG_light[i].u *= SGScale;
        SG_light[i].u[2] -= SGTrans;
    }
    printf("scale: %f\n", SGScale);
    printf("trans: %f\n", SGTrans);

    // sort SG
    std::sort(SG_light, SG_light + state.SGNum, [](const SG &a, const SG &b) {
        return (a.weight[0] + a.weight[1] + a.weight[2]) >
               (b.weight[0] + b.weight[1] + b.weight[2]);
    });
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=

    //  /============\
    // || initialize ||
    //  \============/
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=
    // load background image
    GLint width, height, channel;
    stbi_set_flip_vertically_on_load(true);
    GLubyte *data = stbi_load(backgroundPath, &width, &height, &channel, 0);
    // init glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    // create window
    GLFWwindow *window = glfwCreateWindow(width, height, "Demo", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyboardCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    // init glew
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        puts((const char *)glewGetErrorString(err));
        glfwTerminate();
        return -1;
    }
    // enable depth test
    glEnable(GL_DEPTH_TEST);
    // set blend function
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // enable cube map seamless
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    // create background texture
    GLuint background;
    glGenTextures(1, &background);
    glBindTexture(GL_TEXTURE_2D, background);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    puts("load background ok!");
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=

    //  /=============\
    // || photo depth ||
    //  \=============/
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=
    GLuint depth;
    glGenTextures(1, &depth);
    glBindTexture(GL_TEXTURE_2D, depth);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLint depthWidth, depthHeight;
    stbi_set_flip_vertically_on_load(true);
    data = stbi_load(depthPath, &depthWidth, &depthHeight, &channel, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, depthWidth, depthHeight, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    puts("load depth ok!");
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=

    //  /=============\
    // || config       ||
    //  \=============/
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=
    GLuint SgConfigTex;
    glGenTextures(1, &SgConfigTex);
    glBindTexture(GL_TEXTURE_2D, SgConfigTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // stbi_set_flip_vertically_on_load(true);
    GLfloat *configData = new GLfloat[128 * 9];
    setConfigImg(configData);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 384, 1, 0, GL_RGB, GL_FLOAT,
                 configData);
    puts("load configData ok!");
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=

    //  /==========\
    // || g-buffer ||
    //  \==========/
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=
    GLuint gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    GLuint gPosTex;
    glGenTextures(1, &gPosTex);
    glBindTexture(GL_TEXTURE_2D, gPosTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB,
                 GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           gPosTex, 0);
    GLuint gNormalTex;
    glGenTextures(1, &gNormalTex);
    glBindTexture(GL_TEXTURE_2D, gNormalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB,
                 GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                           gNormalTex, 0);
    GLuint gDiffuseTex;
    glGenTextures(1, &gDiffuseTex);
    glBindTexture(GL_TEXTURE_2D, gDiffuseTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
                           gDiffuseTex, 0);
    GLuint gSpecularTex;
    glGenTextures(1, &gSpecularTex);
    glBindTexture(GL_TEXTURE_2D, gSpecularTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D,
                           gSpecularTex, 0);
    GLuint gMaskTex;
    glGenTextures(1, &gMaskTex);
    glBindTexture(GL_TEXTURE_2D, gMaskTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RGB, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D,
                           gMaskTex, 0);
    GLuint gMaterialTex;
    glGenTextures(1, &gMaterialTex);
    glBindTexture(GL_TEXTURE_2D, gMaterialTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_2D,
                           gMaterialTex, 0);
    // attach
    GLuint attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                            GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
                            GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5};
    glDrawBuffers(6, attachments);
    // create RBO
    GLuint RBO;
    glGenRenderbuffers(1, &RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, RBO);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    puts("create g-buffer ok!");
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=

    //  /=============\
    // || SSAO buffer ||
    //  \=============/
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=
    GLuint ssaoFBO, ssaoBlurFBO;
    glGenFramebuffers(1, &ssaoFBO);
    glGenFramebuffers(1, &ssaoBlurFBO);
    GLuint ssaoTex, ssaoBlurTex;
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glGenTextures(1, &ssaoTex);
    glBindTexture(GL_TEXTURE_2D, ssaoTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RGB, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           ssaoTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glGenTextures(1, &ssaoBlurTex);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RGB, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           ssaoBlurTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=

    //  /=============\
    // || Occ  buffer ||
    //  \=============/
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=
    GLuint occFBO;
    glGenFramebuffers(1, &occFBO);
    GLuint occTex;
    glGenTextures(1, &occTex);
    glBindFramebuffer(GL_FRAMEBUFFER, occFBO);
    glBindTexture(GL_TEXTURE_2D, occTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RGB, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           occTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=

    //  /====================\
    // || SSAO sample kernel ||
    //  \====================/
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=
    std::uniform_real_distribution<GLfloat> rnd(0.0, 1.0);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::vector<glm::vec3> ssaoKernel;
    for (int i = 0; i < 64; i++) {
        glm::vec3 sample(rnd(gen) * 2.0f - 1.0f, rnd(gen) * 2.0f - 1.0f,
                         rnd(gen));
        sample = glm::normalize(sample);
        sample *= rnd(gen);
        float scale = 1.0f * i / 64;
        scale = 0.1f + 0.9f * scale * scale;
        ssaoKernel.push_back(sample * scale);
    }
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=

    //  /============\
    // || SSAO noise ||
    //  \============/
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=
    std::vector<glm::vec3> ssaoNoise;
    for (int i = 0; i < 16; i++) {
        glm::vec3 noise(rnd(gen) * 2.0f - 1.0f, rnd(gen) * 2.0f - 1.0f, 0.0f);
        ssaoNoise.push_back(noise);
    }
    GLuint noiseTex;
    glGenTextures(1, &noiseTex);
    glBindTexture(GL_TEXTURE_2D, noiseTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT,
                 &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    puts("SSAO init ok!");
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=

    //  /============\
    // || shadow map ||
    //  \============/
    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=
    const int shadowMapWidth = 1024, shadowMapHeight = 1024;
    GLuint shadowFBO[4];
    glGenFramebuffers(4, shadowFBO);
    GLuint shadowMap;
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D_ARRAY, shadowMap);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, shadowMapWidth,
                 shadowMapHeight, 4, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
    for (int i = 0; i < 4; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, shadowMap, 0);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  shadowMap, 0, i);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    puts("create shadow map ok!");

    // =.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=.=

    // load shader
    Shader geometryShader("./shader/geometry.vs", "./shader/geometry.fs");
    Shader ssaoShader("./shader/ssao.vs", "./shader/ssao.fs");
    ssaoShader.use();
    ssaoShader.setInt("gPos", 0);
    ssaoShader.setInt("gNormal", 1);
    ssaoShader.setInt("texNoise", 2);
    ssaoShader.setInt("gMask", 3);
    ssaoShader.setVec2("noiseScale", glm::vec2(width / 4.0f, height / 4.0f));
    Shader ssaoBlurShader("./shader/ssao.vs", "./shader/ssaoblur.fs");
    ssaoBlurShader.use();
    ssaoBlurShader.setInt("ssaoMap", 0);
    Shader lightingShader("./shader/lighting.vs", "./shader/lighting.fs");
    lightingShader.use();
    for (int i = 0; i < state.SGNum; i++) {
        char name[32];
        sprintf(name, "SG_light[%d].weight", i);
        lightingShader.setVec3(name, SG_light[i].weight);
        sprintf(name, "SG_light[%d].u", i);
        SG_light[i].u = glm::normalize(SG_light[i].u);
        lightingShader.setVec3(name, SG_light[i].u);
        sprintf(name, "SG_light[%d].alpha", i);
        lightingShader.setFloat(name, SG_light[i].alpha);
    }
    // for (int i = 0; i < 16; i++) {
    //     char name[32];
    //     sprintf(name, "SH_light[%d].weight", i);
    //     lightingShader.setVec3(name, SH_light[i].weight);
    // }
    lightingShader.setInt("ssaoMap", 0);
    lightingShader.setInt("gPos", 1);
    lightingShader.setInt("gNormal", 2);
    lightingShader.setInt("gDiffuse", 3);
    lightingShader.setInt("gSpecular", 4);
    lightingShader.setInt("shadowMap", 5);
    lightingShader.setInt("occMap", 6);
    lightingShader.setInt("background", 7);
    lightingShader.setInt("gMask", 8);
    lightingShader.setInt("gMaterial", 9);
    lightingShader.setInt("depth", 10);
    Shader depthShader("./shader/shadowmap.vs", "./shader/shadowmap.fs");
    Shader occShader("./shader/lighting.vs", "./shader/occ.fs");
    occShader.use();
    // occShader.setInt("depth", 0);
    puts("load shader ok!");

    // restore view port
    int scrWidth, scrHeight;
    glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
    glViewport(0, 0, scrWidth, scrHeight);
    puts("restore view port ok!");

    // only load model 0
    modelState[0].model = new Model(modelPaths[0]);
    puts("load model ok!");
    modelState[0].modelCenter = (modelState[0].model->getAABB().minCoords +
                                 modelState[0].model->getAABB().maxCoords) *
                                0.5f;
    modelState[0].moveToOri =
        glm::translate(glm::mat4(1.0f), -modelState[0].modelCenter);
    glm::vec3 modelDiff = modelState[0].model->getAABB().maxCoords -
                          modelState[0].model->getAABB().minCoords;
    float scaleRatio =
        5.0f / (std::max(modelDiff.x, std::max(modelDiff.y, modelDiff.z)));
    modelState[0].rescale = glm::scale(glm::mat4(1.0f), glm::vec3(scaleRatio));

    // shadow bottom surface
    GLfloat surfaceVertices[] = {
        -100.0f, 0.0f, -100.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        -100.0f, 0.0f, +100.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        +100.0f, 0.0f, -100.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        -100.0f, 0.0f, +100.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        +100.0f, 0.0f, +100.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        +100.0f, 0.0f, -100.0f, 0.0f, 1.0f, 0.0f, 1.0f};
    GLuint surfaceVAO, surfaceVBO;
    glGenVertexArrays(1, &surfaceVAO);
    glGenBuffers(1, &surfaceVBO);
    glBindVertexArray(surfaceVAO);
    glBindBuffer(GL_ARRAY_BUFFER, surfaceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(surfaceVertices), surfaceVertices,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat),
                          nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat),
                          (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat),
                          (void *)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // shadow back surface
    GLfloat backSurfaceVertices[] = {
        0.0f, -100.0f, +100.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, +100.0f, +100.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, +100.0f, -100.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, +100.0f, -100.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, -100.0f, -100.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, -100.0f, +100.0f, -1.0f, 0.0f, 0.0f, 1.0f};
    glGenVertexArrays(1, &modelState[0].backSurfaceVAO);
    glGenBuffers(1, &modelState[0].backSurfaceVBO);
    glBindVertexArray(modelState[0].backSurfaceVAO);
    glBindBuffer(GL_ARRAY_BUFFER, modelState[0].backSurfaceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(backSurfaceVertices),
                 backSurfaceVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat),
                          nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat),
                          (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat),
                          (void *)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // camera configuration
    glm::vec3 eye(-15.0f, 5.0f, 0.0f);
    glm::mat4 view = glm::lookAt(eye,
                                 glm::vec3(-15.0f + std::cos(elevation),
                                           5.0f + std::sin(elevation), 0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    state.invView = glm::inverse(glm::mat3(view));
    glm::mat4 projection =
        glm::perspective(fov, 1.0f * scrWidth / scrHeight, 0.1f, 100.0f);

    // light configuration
    glm::vec3 lightDir[3];
    glm::mat4 lightProjection =
        glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 100.0f);
    glm::mat4 lightMat[3];
    float lightWeight[3];
    float totWeight = 0.0;

    // combineLight();

    // main loop
    while (!glfwWindowShouldClose(window)) {
        for (int i = 0; i < 3; i++) {
            glm::vec3 objPos = glm::vec3(modelState[0].lightRela * glm::vec4(0, 0, 0, 1));
            glm::vec3 tempv = SG_light[i].u;
            tempv.y += 1.0f;
            tempv = glm::vec3(modelState[0].lightRot*glm::vec4(tempv,1));
            lightDir[i] = glm::normalize(tempv);
        }

        // calculate model matrix
        glm::mat4 model = modelState[0].objMat * modelState[0].lightRot * modelState[0].objRotMat *
                          modelState[0].rescale * modelState[0].moveToOri;


        glm::vec4 modelCenter = projection * view * model * glm::vec4(modelState[0].modelCenter, 1.0f);
        glm::vec2 texCod = glm::vec2(modelCenter[0]/modelCenter[2]+0.5, modelCenter[1]/modelCenter[2]+0.5);

        // step 1. shadow pass
        glViewport(0, 0, shadowMapWidth, shadowMapHeight);
        for (int i = 0; i < 3; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO[i]);
            glClear(GL_DEPTH_BUFFER_BIT);
            glCullFace(GL_FRONT);
            depthShader.use();
            // if there are multiple objects, please calculate their respective
            // newCenters. Then, use the geometric center of these newCenters to
            // calculate lightView.
            glm::vec4 newCenter = glm::vec4(modelState[0].modelCenter, 1.0f);
            newCenter = model * newCenter;
            glm::mat4 lightView =
                glm::lookAt(lightDir[i] * 15.0f+glm::vec3(newCenter),
                            glm::vec3(newCenter), glm::vec3(0.0f, 1.0f, 0.0f));
            // if(i == 0)
            //     printf("%f %f %f\n", lightDir[i][0], lightDir[i][1], lightDir[i][2]);
            lightMat[i] = lightProjection * lightView;
            depthShader.setMat4("lightMat", lightMat[i]);
            depthShader.setMat4("model", model);
            modelState[0].model->draw(depthShader);
            // only draw bottom surface !!!
            depthShader.setMat4("model", state.surfaceMat);
            glBindVertexArray(surfaceVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glCullFace(GL_BACK);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        glViewport(0, 0, scrWidth, scrHeight);

        // step 2. geometry pass
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        geometryShader.use();
        geometryShader.setMat4("model", model);
        geometryShader.setMat4("view", view);
        geometryShader.setMat4("projection", projection);
        // set material
        geometryShader.setFloat("metallic", 0.4f);
        geometryShader.setFloat("roughness", 0.2f);
        modelState[0].model->draw(geometryShader);
        geometryShader.setMat4("model", state.surfaceMat);
        glBindVertexArray(surfaceVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // geometryShader.setMat4("model", modelState[0].objMat *
        // modelState[0].objRotMat * modelState[0].moveToOri);
        // glBindVertexArray(modelState[0].backSurfaceVAO);
        // glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // step 3. SSAO pass
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        ssaoShader.use();
        char name[32];
        for (int i = 0; i < 64; i++) {
            sprintf(name, "samples[%d]", i);
            ssaoShader.setVec3(name, ssaoKernel[i]);
        }
        ssaoShader.setMat4("view", view);
        ssaoShader.setMat4("projection", projection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormalTex);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, noiseTex);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, gMaskTex);
        renderQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // step 4. SSAO blur pass
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        ssaoBlurShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssaoTex);
        renderQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // step 4.5 Occ pass
        glBindFramebuffer(GL_FRAMEBUFFER, occFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        occShader.use();
        // occShader.setVec2("rayDir", glm::vec2(-1.0f, 1.0f));
        glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, depth);
        renderQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // calc occ ratio
        float *pOcc = new float[width * height];
        float *pMask = new float[width * height];
        float *pNormal = new float[width * height * 3];
        glBindTexture(GL_TEXTURE_2D, occTex);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, pOcc);
        glBindTexture(GL_TEXTURE_2D, gMaskTex);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, pMask);
        glBindTexture(GL_TEXTURE_2D, gNormalTex);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, pNormal);
        float occArea = 0.0f, totArea = 0.0f;
        for (int i = 0; i < width * height; i++) {
            if (*pMask == 0.0f && (pNormal[0] != 0.0f || pNormal[1] != 0.0f ||
                                   pNormal[2] != 0.0f)) {
                totArea += 1.0f;
                occArea += *pOcc;
            }
            pMask++;
            pOcc++;
            pNormal += 3;
        }
        pMask -= width * height;
        pOcc -= width * height;
        pNormal -= width * height * 3;

        delete[] pMask;
        delete[] pOcc;
        delete[] pNormal;

        // step 5. lighting pass
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        lightingShader.use();
        lightingShader.setFloat("scale", 5.0f * (1.0 - occArea / totArea));
        lightingShader.setFloat("surfaceY", state.surfaceY);
        lightingShader.setInt("dispSurface", state.dispSurface);
        lightingShader.setVec3("eye", eye);
        lightingShader.setVec2("objCenter", texCod);

        glm::vec3 objPos = glm::vec3(modelState[0].lightRela * glm::vec4(0, 0, 0, 1));
        // printf("objPos: %f %f %f %f\n", objPos[0], objPos[1], objPos[2], glm::length((SG_light[0].u * glm::vec3(8, 8, 8)) - objPos));
        // printf("light_pos: %f %f %f\n", SG_light[0].u[0], SG_light[0].u[1],
        // SG_light[0].u[2]); glm::vec3 temp = SG_light[0].u *
        // glm::length(SG_light[0].u-objPos); printf("light_pos after u: %f %f
        // %f\n", temp[0], temp[1], temp[2]); temp = glm::normalize(temp);
        // printf("light_pos after u norm: %f %f %f\n", temp[0], temp[1],
        // temp[2]);

        for (int i = 0; i < state.SGNum; i++) {
            char name[32];
            sprintf(name, "SG_light[%d].u", i);
            lightingShader.setVec3(name, glm::vec3(modelState[0].lightRot*glm::vec4(SG_light[i].u,1)));
        }
        for (int i = 0; i < 3; i++) {
            char name[32];
            sprintf(name, "lightDir[%d]", i);
            lightingShader.setVec3(name, lightDir[i]);
            //            sprintf(name, "lightWeight[%d]", i);
            //            lightingShader.setFloat(name, lightWeight[i]);
            sprintf(name, "lightMat[%d]", i);
            lightingShader.setMat4(name, lightMat[i]);
        }
        lightingShader.setMat4("projView", projection * view);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssaoBlurTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gPosTex);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gNormalTex);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, gDiffuseTex);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, gSpecularTex);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D_ARRAY, shadowMap);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, occTex);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, background);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, gMaskTex);
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, gMaterialTex);
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, depth);

        renderQuad();

        // exportImage(window, save_path);
        glfwSwapBuffers(window);
        glfwPollEvents();

        // break;
    }

    glDeleteBuffers(1, &surfaceVBO);
    glDeleteVertexArrays(1, &surfaceVAO);

    glfwTerminate();
    return 0;
}

/*
void combineLight()
{
    for (;;)
    {
        float dis = -1e9;
        int ii, jj;
        for (size_t i = 0; i < 16; i++)
            for (size_t j = i + 1; j < 16; j++)
                if (light[i].weight[0] + light[j].weight[0] > 0 &&
glm::dot(light[i].u, light[j].u) > dis)
                {
                    dis = glm::dot(light[i].u, light[j].u);
                    ii = i, jj = j;
                }
        if (dis < 0.9) break;
        SG sg;
        sg.weight = light[ii].weight + light[jj].weight;
        sg.u = (light[ii].weight[0] + light[ii].weight[1] + light[ii].weight[2])
* light[ii].u + (light[jj].weight[0] + light[jj].weight[1] +
light[jj].weight[2]) * light[jj].u; sg.u = glm::normalize(sg.u); sg.alpha =
light[ii].alpha; light[ii] = sg, light[jj].weight = glm::vec3(-1e5);
    }
}
*/

void renderQuad() {
    static GLuint quadVAO = 0u, quadVBO = 0u;
    if (!quadVAO) {
        GLfloat vertices[] = {
            // positions        // texture Coords
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                     GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                              nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                              (void *)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            modelState[0].lastRotVec = trackBall(x, y, w, h);
            state.isPressed = true;
        } else if (action == GLFW_RELEASE)
            state.isPressed = false;
    }
}

void cursorPositionCallback(GLFWwindow *window, double x, double y) {
    if (state.isPressed) {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glm::vec3 rotVec = trackBall(x, y, w, h);
        glm::vec3 dVec = rotVec - modelState[0].lastRotVec;
        if (glm::length(dVec) > 1e-7f) {
            modelState[0].objRotMat =
                glm::rotate(glm::mat4(1.0f),
                            glm::pi<float>() / 2.0f * glm::length(dVec),
                            glm::cross(modelState[0].lastRotVec, rotVec)) *
                modelState[0].objRotMat;
            modelState[0].lastRotVec = rotVec;
        }
    }
}

void keyboardCallback(GLFWwindow *window, int key, int scanCode, int action,
                      int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_W:  // move front
                modelState[0].objMat = glm::translate(
                    modelState[0].objMat, glm::vec3(0.1f, 0.0f, 0.0f));
                modelState[0].lightRela = glm::translate(
                    modelState[0].lightRela, glm::vec3(0.1f, 0.0f, 0.0f));
                break;
            case GLFW_KEY_A:  // move left
                modelState[0].objMat = glm::translate(
                    modelState[0].objMat, glm::vec3(0.0f, 0.0f, -0.1f));
                modelState[0].lightRela = glm::translate(
                    modelState[0].lightRela, glm::vec3(0.0f, 0.0f, -0.1f));
                break;
            case GLFW_KEY_S:  // move back
                modelState[0].objMat = glm::translate(
                    modelState[0].objMat, glm::vec3(-0.1f, 0.0f, 0.0f));
                modelState[0].lightRela = glm::translate(
                    modelState[0].lightRela, glm::vec3(-0.1f, 0.0f, 0.0f));
                break;
            case GLFW_KEY_D:  // move right
                modelState[0].objMat = glm::translate(
                    modelState[0].objMat, glm::vec3(0.0f, 0.0f, 0.1f));
                modelState[0].lightRela = glm::translate(
                    modelState[0].lightRela, glm::vec3(0.0f, 0.0f, 0.1f));
                break;
            case GLFW_KEY_Q:  // rotate
                modelState[0].lightRot = glm::rotate(modelState[0].lightRot, glm::radians(1.0f), glm::vec3(0.0, 1.0, 0.0));
                break;
            case GLFW_KEY_E:  // rotate
                modelState[0].lightRot = glm::rotate(modelState[0].lightRot, glm::radians(-1.0f), glm::vec3(0.0, 1.0, 0.0));
                break;
            case GLFW_KEY_UP:
                if (mods == GLFW_MOD_CONTROL)  // move up
                    modelState[0].objMat = glm::translate(
                        modelState[0].objMat, glm::vec3(0.0f, 0.1f, 0.0f));
                else  // shadow surface up
                {
                    state.surfaceMat = glm::translate(
                        state.surfaceMat, glm::vec3(0.0f, 0.1f, 0.0f));
                    state.surfaceY += 0.1f;
                }
                break;
            case GLFW_KEY_DOWN:
                if (mods == GLFW_MOD_CONTROL)  // move down
                    modelState[0].objMat = glm::translate(
                        modelState[0].objMat, glm::vec3(0.0f, -0.1f, 0.0f));
                else  // shadow surface down
                {
                    state.surfaceMat = glm::translate(
                        state.surfaceMat, glm::vec3(0.0f, -0.1f, 0.0f));
                    state.surfaceY += -0.1f;
                }
                break;
            case GLFW_KEY_LEFT:  // scale down
                modelState[0].objMat = glm::scale(
                    modelState[0].objMat, glm::vec3(0.95f, 0.95f, 0.95f));
                break;
            case GLFW_KEY_RIGHT:  // scale up
                modelState[0].objMat = glm::scale(
                    modelState[0].objMat, glm::vec3(1.05f, 1.05f, 1.05f));
                break;
            case GLFW_KEY_O:
                state.dispSurface = !state.dispSurface;
                break;
            case GLFW_KEY_P:
                puts("objRotMat:");
                for (int i = 0; i < 16; i++)
                    printf("%.8f ", *(&modelState[0].objRotMat[0][0] + i));
                puts("");
                puts("objMat:");
                for (int i = 0; i < 16; i++)
                    printf("%.8f ", *(&modelState[0].objMat[0][0] + i));
                puts("");
                puts("sfMat:");
                for (int i = 0; i < 16; i++)
                    printf("%.8f ", *(&state.surfaceMat[0][0] + i));
                puts("");
                puts("sfY:");
                printf("%.8f\n", state.surfaceY);
                break;
            case GLFW_KEY_B:
                exportImage(window);
                break;
        }
    }
}
