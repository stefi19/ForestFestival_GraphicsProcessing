#ifndef SkyBox_hpp
#define SkyBox_hpp

#if defined (__APPLE__)
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <glm/glm.hpp>
#include <vector>
#include "Shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "stb_image.h"

namespace gps {

    class SkyBox {
    public:
        SkyBox();
        void Load(const std::vector<const GLchar*> &cubeMapFaces);
        void Draw(gps::Shader shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
        GLuint GetTextureId();
    private:
        GLuint skyboxVAO;
        GLuint skyboxVBO;
        GLuint cubemapTexture;
        GLuint LoadSkyBoxTextures(const std::vector<const GLchar*> &cubeMapFaces);
        void InitSkyBox();
    };

}

#endif /* SkyBox_hpp */
