#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types
#include <glm/gtc/constants.hpp>

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
        // Balloons skipped (not loaded)
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
// fog uniform locations
GLint fogColorLoc;
GLint fogDensityLoc;
GLint fogRadiusLoc;
GLint fogRadiusXLoc;
GLint hatCenterLoc;
GLint fogStretchLoc;
GLint fogEnabledLoc;
GLint fogTimeLoc;
GLint flatShadingLoc = -1;
// spotlight uniform locations (arrays 2)
GLint spotPosLoc[2];
GLint spotDirLoc[2];
GLint spotConstLoc[2];
GLint spotLinearLoc[2];
GLint spotQuadLoc[2];
GLint spotCutoffLoc[2];
GLint spotIntensityLoc[2];
// current fog parameter copies for debugging
float currentFogDensity = 0.0f;
float currentFogRadius = 0.0f;
float currentFogStretchDown = 0.0f;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 3.0f, 20.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

// clap animation state
bool clapActive = false;
float clapOffset = 0.0f;
float clapSpeed = 0.015f; // units per frame (slower for clearer observation)
// based on the provided palm positions (~0.7m apart), use half-distance per-hand
float clapMax = 0.35f; // maximum per-hand translation before reversing
int clapDirection = 1; // 1 = moving inward, -1 = moving outward

GLfloat cameraSpeed = 0.1f;

// cinematic camera presentation
bool cinematicActive = false;
float cinematicTime = 0.0f;
// phases: 0=descend,1=rabbit appear+clap,2=hold,3=orbit,4=return,5=done
int cinematicPhase = -1;
glm::vec3 cinematic_savedPos;
glm::vec3 cinematic_savedTarget;

// timing (seconds)
const float CIN_DESCEND = 4.0f;
const float CIN_RABBIT = 1.5f;
const float CIN_HOLD = 2.0f;
const float CIN_ORBIT = 8.0f;
const float CIN_RETURN = 3.0f;
const float CIN_HANDS_FOCUS = 5.0f;
const float CIN_HANDS_MOVE = 0.8f; // time to move camera to hands focus

// exploration (phase 3) state
int cinematicExploreIndex = 0;
glm::vec3 cinematicExploreStartPos = glm::vec3(0.0f);
const float CIN_EXPLORE_PER = 3.0f; // seconds per focus
glm::vec3 cinematicHandsStartPos = glm::vec3(0.0f);


// rabbit-from-hat animation state (instant toggle: 0 or 1)
float rabbitScale = 1.0f;

// debug flag to request mesh info prints (defined in gps namespace to match extern)
bool gps::g_debugPrintMeshInfo = false;

GLboolean pressedKeys[1024];

// render modes
enum RenderMode { RENDER_SOLID = 0, RENDER_WIREFRAME = 1, RENDER_POLYGONAL = 2, RENDER_SMOOTH = 3 };
RenderMode currentRenderMode = RENDER_SOLID;

// mouse control
double lastX = 0.0;
double lastY = 0.0;
bool firstMouse = true;
float mouseSensitivity = 0.1f; // degrees per pixel

// models
//gps::Model3D TeapotModel;
gps::Model3D FerisWheelModel;
gps::Model3D HatModel;
gps::Model3D IceCreamModel;
gps::Model3D LeftHandsModel;
gps::Model3D PlaygroundModel;
gps::Model3D RabbitModel;
gps::Model3D RightHandsModel;
gps::Model3D SceneModel;
gps::Model3D SwingModel;
gps::Model3D WheelModel;
gps::Model3D TreesModel;
GLfloat angle;

// shaders
gps::Shader myBasicShader;
gps::Shader rainShader;
// skybox
gps::SkyBox mySkyBox;
gps::Shader skyboxShader;

// rain fullscreen quad
GLuint rainQuadVAO = 0;
GLuint rainQuadVBO = 0;
GLint rainTimeLoc = -1;
GLint rainCamPosLoc = -1;
GLint rainIntensityLoc = -1;
GLint rainColorLoc = -1;
float rainIntensity = 0.15f;
glm::vec3 rainColor = glm::vec3(0.6f, 0.6f, 0.9f);

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	//TODO
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
            // render mode keys
            if (key == GLFW_KEY_7) {
                currentRenderMode = RENDER_SOLID;
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                std::cout << "Render mode: SOLID" << std::endl;
            }
            if (key == GLFW_KEY_8) {
                currentRenderMode = RENDER_WIREFRAME;
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                std::cout << "Render mode: WIREFRAME" << std::endl;
            }
            if (key == GLFW_KEY_9) {
                currentRenderMode = RENDER_POLYGONAL;
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                std::cout << "Render mode: POLYGONAL (flat)" << std::endl;
                // immediately upload flatShading uniform
                myBasicShader.useShaderProgram();
                if (flatShadingLoc != -1) glUniform1i(flatShadingLoc, 1);
            }
            if (key == GLFW_KEY_0) {
                currentRenderMode = RENDER_SMOOTH;
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                std::cout << "Render mode: SMOOTH" << std::endl;
                // immediately upload flatShading uniform
                myBasicShader.useShaderProgram();
                if (flatShadingLoc != -1) glUniform1i(flatShadingLoc, 0);
            }
            // alternate keys (function keys) in case number row isn't captured
            if (key == GLFW_KEY_F7) { currentRenderMode = RENDER_SOLID; glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); std::cout<<"Render mode: SOLID (F7)"<<std::endl; }
            if (key == GLFW_KEY_F8) { currentRenderMode = RENDER_WIREFRAME; glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); std::cout<<"Render mode: WIREFRAME (F8)"<<std::endl; }
            if (key == GLFW_KEY_F9) { currentRenderMode = RENDER_POLYGONAL; glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); myBasicShader.useShaderProgram(); if (flatShadingLoc!=-1) glUniform1i(flatShadingLoc,1); std::cout<<"Render mode: POLYGONAL (F9)"<<std::endl; }
            if (key == GLFW_KEY_F10) { currentRenderMode = RENDER_SMOOTH; glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); myBasicShader.useShaderProgram(); if (flatShadingLoc!=-1) glUniform1i(flatShadingLoc,0); std::cout<<"Render mode: SMOOTH (F10)"<<std::endl; }
            if (key == GLFW_KEY_P) { // toggle clap animation
                clapActive = !clapActive;
                if (!clapActive) { clapOffset = 0.0f; clapDirection = 1; } // reset when turned off
            }
            if (key == GLFW_KEY_C && action == GLFW_PRESS) {
                // start cinematic presentation
                if (!cinematicActive) {
                    cinematicActive = true;
                    cinematicTime = 0.0f;
                    cinematicPhase = 0;
                    // save current camera state
                    cinematic_savedPos = myCamera.getPosition();
                    // approximate current look target by transforming view-space forward into world
                    glm::mat4 v = myCamera.getViewMatrix();
                    glm::mat4 invv = glm::inverse(v);
                    glm::vec4 targetInWorld = invv * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);
                    cinematic_savedTarget = glm::vec3(targetInWorld);
                    // reset exploration state so repeated 'C' runs behave the same
                    cinematicExploreIndex = 0;
                    cinematicExploreStartPos = myCamera.getPosition();
                    // ensure rabbit will animate in during this cinematic
                    rabbitScale = 0.0f;
                }
            }
            if (key == GLFW_KEY_I) { // toggle rabbit appearance from hat (instant)
                if (rabbitScale > 0.0f) {
                    rabbitScale = 0.0f; // hide immediately
                } else {
                    rabbitScale = 1.0f; // show immediately
                }
            }
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top

    lastX = xpos;
    lastY = ypos;

    float xoff = (float)xoffset * mouseSensitivity;
    float yoff = (float)yoffset * mouseSensitivity;

    // pitch, yaw (rotate expects pitch then yaw)
    myCamera.rotate(yoff, xoff);
    // update view uniform
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        glm::vec3 pos = myCamera.getPosition();
        std::cout << "Camera X: " << pos.x << " Y: " << pos.y << " Z: " << pos.z << std::endl;
    }
}

void processMovement() {
    if (cinematicActive) return; // disable manual movement during cinematic
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        // update view matrix for all models
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	}

    if (pressedKeys[GLFW_KEY_UP]) {
        myCamera.move(gps::MOVE_UP, cameraSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        // update view matrix for all models
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	}

    if (pressedKeys[GLFW_KEY_DOWN]) {
        myCamera.move(gps::MOVE_DOWN, cameraSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        // update view matrix for all models
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        // update view matrix for all models
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	}

    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }

    // update clap animation each frame
    if (clapActive) {
        clapOffset += clapSpeed * (float)clapDirection;
        if (clapOffset >= clapMax) {
            clapOffset = clapMax;
            clapDirection = -1;
        } else if (clapOffset <= 0.0f) {
            clapOffset = 0.0f;
            clapDirection = 1;
        }
    } else {
        clapOffset = 0.0f;
        clapDirection = 1;
    }
        // rabbitScale is an instant toggle (no per-frame fading)
}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetMouseButtonCallback(myWindow.getWindow(), mouseButtonCallback);
    // capture and hide the cursor for mouse look
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void initOpenGLState() {
    // match scene clear color to fog base color
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    //TeapotModel.LoadModel("models/teapot/teapot20segUT.obj");
    FerisWheelModel.LoadModel("models/FerisWheel/FerisWhee;.obj");
    HatModel.LoadModel("models/Hat/Hat.obj");
    IceCreamModel.LoadModel("models/IceCream/IceCream.obj");
    LeftHandsModel.LoadModel("models/LeftHands/LeftHands.obj");
    PlaygroundModel.LoadModel("models/Playground/Playground.obj");
    RabbitModel.LoadModel("models/Rabbit/Rabbit.obj");
    RightHandsModel.LoadModel("models/RightHands/RightHands.obj");
    SceneModel.LoadModel("models/Scene/Scene.obj");
    SwingModel.LoadModel("models/Swing/Swing.obj");
    WheelModel.LoadModel("models/Wheel/Wheel.obj");
    TreesModel.LoadModel("models/MoreTrees/NewTrees.obj");

    // debug: print hat model-space center to help position fog
    glm::vec3 hatCenterModel = HatModel.getCenter();
    std::cout << "Hat model center: X:" << hatCenterModel.x << " Y:" << hatCenterModel.y << " Z:" << hatCenterModel.z << std::endl;
}

void initSkybox()
{
    std::vector<const GLchar*> faces;
    // prefer existing JPEG faces when available
    faces.push_back("skybox/posx.jpg"); // right
    faces.push_back("skybox/negx.jpg"); // left
    faces.push_back("skybox/posy.jpg"); // top
    faces.push_back("skybox/negy.jpg"); // bottom
    faces.push_back("skybox/posz.jpg"); // back
    faces.push_back("skybox/negz.jpg"); // front

    mySkyBox.Load(faces);
}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");

    // load skybox shader
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    skyboxShader.useShaderProgram();

    // load rain shader
    rainShader.loadShader("shaders/rain.vert", "shaders/rain.frag");
    rainShader.useShaderProgram();
}

void initRain() {
    // fullscreen quad (NDC)
    float quadVertices[] = {
        // positions      // uv
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,

        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &rainQuadVAO);
    glGenBuffers(1, &rainQuadVBO);
    glBindVertexArray(rainQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rainQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    rainTimeLoc = glGetUniformLocation(rainShader.shaderProgram, "time");
    rainCamPosLoc = glGetUniformLocation(rainShader.shaderProgram, "camPos");
    rainIntensityLoc = glGetUniformLocation(rainShader.shaderProgram, "intensity");
    rainColorLoc = glGetUniformLocation(rainShader.shaderProgram, "rainColor");
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    // create model matrix for teapot
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
    projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    // fog defaults
    fogColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogColor");
    fogDensityLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity");
    fogRadiusLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogRadius");
    hatCenterLoc = glGetUniformLocation(myBasicShader.shaderProgram, "hatCenterWorld");

    // tuned fog settings: increased vertical stretch and stronger/wider horizontal
    // user-requested: magenta, slightly denser, thin depth, extended left/right and slightly taller upward
    glm::vec3 fogColor = glm::vec3(1.0f, 0.0f, 1.0f);
    float fogDensity = 1.10f; // increased density (clamped in shader)
    float fogRadius = 3.5f; // depth (Z) radius - keep small
    float fogRadiusX = 9.0f; // left/right (X) radius - extended more
    float fogStretchDown = 2.0f; // keep tall vertical stretch

    glUniform3fv(fogColorLoc, 1, glm::value_ptr(fogColor));
    glUniform1f(fogDensityLoc, fogDensity);
    glUniform1f(fogRadiusLoc, fogRadius);
    fogRadiusXLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogRadiusX");
    if (fogRadiusXLoc != -1) glUniform1f(fogRadiusXLoc, fogRadiusX);
    fogStretchLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogStretchDown");
    if (fogStretchLoc != -1) glUniform1f(fogStretchLoc, fogStretchDown);
    fogEnabledLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogEnabled");
    if (fogEnabledLoc != -1) glUniform1i(fogEnabledLoc, 1);
    // time uniform for animated fog
    fogTimeLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogTime");
    if (fogTimeLoc != -1) glUniform1f(fogTimeLoc, 0.0f);
    // flat/ polygonal shading uniform
    flatShadingLoc = glGetUniformLocation(myBasicShader.shaderProgram, "flatShading");
    if (flatShadingLoc != -1) glUniform1i(flatShadingLoc, 0);
    // spotlight uniform locations and defaults (only 2 outer spotlights)
    for (int i = 0; i < 2; ++i) {
        std::string idx = std::to_string(i);
        spotPosLoc[i] = glGetUniformLocation(myBasicShader.shaderProgram, ("spotPos[" + idx + "]").c_str());
        spotDirLoc[i] = glGetUniformLocation(myBasicShader.shaderProgram, ("spotDir[" + idx + "]").c_str());
        spotConstLoc[i] = glGetUniformLocation(myBasicShader.shaderProgram, ("spotConstant[" + idx + "]").c_str());
        spotLinearLoc[i] = glGetUniformLocation(myBasicShader.shaderProgram, ("spotLinear[" + idx + "]").c_str());
        spotQuadLoc[i] = glGetUniformLocation(myBasicShader.shaderProgram, ("spotQuadratic[" + idx + "]").c_str());
        spotCutoffLoc[i] = glGetUniformLocation(myBasicShader.shaderProgram, ("spotCutoffCos[" + idx + "]").c_str());
        spotIntensityLoc[i] = glGetUniformLocation(myBasicShader.shaderProgram, ("spotIntensity[" + idx + "]").c_str());
        // set some safe defaults (can be overridden per-frame)
        if (spotConstLoc[i] != -1) glUniform1f(spotConstLoc[i], 1.0f);
        if (spotLinearLoc[i] != -1) glUniform1f(spotLinearLoc[i], 0.0045f);
        if (spotQuadLoc[i] != -1) glUniform1f(spotQuadLoc[i], 0.0075f);
        if (spotCutoffLoc[i] != -1) glUniform1f(spotCutoffLoc[i], cos(glm::radians(25.0f))); // ~25deg cone
        if (spotIntensityLoc[i] != -1) glUniform1f(spotIntensityLoc[i], 3.0f); // default stronger intensity
    }
    // store current fog params for debugging
    currentFogDensity = fogDensity;
    currentFogRadius = fogRadius;
    currentFogStretchDown = fogStretchDown;
}

void renderModels(gps::Shader shader) {
    shader.useShaderProgram();

    // apply global render mode settings for this shader pass
    if (currentRenderMode == RENDER_WIREFRAME) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    // compute flat shading flag to forward to draws (meshes will set uniform after binding)
    int flatFlag = (currentRenderMode == RENDER_POLYGONAL) ? 1 : 0;

    // update hat center uniform (compute in world space)
    // compute hat bounds in world space and set fog center at mid-height of hat
    glm::vec3 hatMinModel = HatModel.getMinBounds();
    glm::vec3 hatMaxModel = HatModel.getMaxBounds();
    glm::vec3 hatMinWorld = glm::vec3(model * glm::vec4(hatMinModel, 1.0f));
    glm::vec3 hatMaxWorld = glm::vec3(model * glm::vec4(hatMaxModel, 1.0f));
    glm::vec3 hatCenterWorld = glm::vec3((hatMinWorld + hatMaxWorld) * 0.5f);
    // shift center slightly down so fog sits below mid-hat and extends toward the scene
    hatCenterWorld.y -= 0.40f;
    if (hatCenterLoc != -1) glUniform3fv(hatCenterLoc, 1, glm::value_ptr(hatCenterWorld));
    // compute rabbit center in world space for spotlight targeting
    glm::vec3 rabbitCenterModel = RabbitModel.getCenter();
    glm::vec3 rabbitCenterWorld = glm::vec3(model * glm::vec4(rabbitCenterModel, 1.0f));
    // choose a target between hat and rabbit centers
    glm::vec3 spotTarget = (hatCenterWorld + rabbitCenterWorld) * 0.5f;
    // use only the two outermost spotlight origins (leftmost and rightmost)
    glm::vec3 spotOrigins[2] = {
        glm::vec3(-6.7394f, 2.94475f, -20.4938f), // outer-left
        glm::vec3(7.66052f, 2.94475f, -20.506f)   // outer-right
    };
    // upload spot positions and directions and attenuation (per-frame)
    for (int i = 0; i < 2; ++i) {
        if (spotPosLoc[i] != -1) glUniform3fv(spotPosLoc[i], 1, glm::value_ptr(spotOrigins[i]));
        glm::vec3 dir = glm::normalize(spotTarget - spotOrigins[i]);
        if (spotDirLoc[i] != -1) glUniform3fv(spotDirLoc[i], 1, glm::value_ptr(dir));
        // attenuation choices: constant=1.0, linear=0.0045, quadratic=0.0075 (adjustable)
        if (spotConstLoc[i] != -1) glUniform1f(spotConstLoc[i], 1.0f);
        if (spotLinearLoc[i] != -1) glUniform1f(spotLinearLoc[i], 0.0045f);
        if (spotQuadLoc[i] != -1) glUniform1f(spotQuadLoc[i], 0.0075f);
        // cutoff: keep relatively tight cone
        if (spotCutoffLoc[i] != -1) glUniform1f(spotCutoffLoc[i], cos(glm::radians(25.0f)));
        if (spotIntensityLoc[i] != -1) glUniform1f(spotIntensityLoc[i], 3.0f);
    }
    // update animated fog time uniform
    if (fogTimeLoc != -1) {
        float t = (float)glfwGetTime();
        glUniform1f(fogTimeLoc, t);
    }
    // set flat shading uniform based on current render mode
    if (flatShadingLoc != -1) {
        if (currentRenderMode == RENDER_POLYGONAL) glUniform1i(flatShadingLoc, 1);
        else glUniform1i(flatShadingLoc, 0);
    }
    // debug: occasionally print hat world center (disabled by default)
    // std::cout << "Hat world center: " << hatCenterWorld.x << ", " << hatCenterWorld.y << ", " << hatCenterWorld.z << std::endl;
    // periodic debug print of hat/fog params
        // periodic fog debug print disabled (set to false to enable)
        if (false) {
            static double lastPrint = 0.0;
            double now = glfwGetTime();
            if (now - lastPrint > 1.0) {
                lastPrint = now;
                std::cout << "[FogDebug] hatCenterWorld=" << hatCenterWorld.x << "," << hatCenterWorld.y << "," << hatCenterWorld.z
                    << " fogRadius=" << currentFogRadius << " fogDensity=" << currentFogDensity
                    << " fogStretchDown=" << currentFogStretchDown << std::endl;
            }
        }

    // shared model transform used for now; compute normal matrix per draw
    /*glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    TeapotModel.Draw(shader);*/

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glm::mat3 nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));


    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    FerisWheelModel.Draw(shader, flatFlag);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    // disable fog when drawing the hat itself so texture isn't fogged
    if (fogEnabledLoc != -1) glUniform1i(fogEnabledLoc, 0);
    HatModel.Draw(shader, flatFlag);
    if (fogEnabledLoc != -1) glUniform1i(fogEnabledLoc, 1);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    IceCreamModel.Draw(shader, flatFlag);

    // Left hand - apply clap translation
    {
        glm::mat4 leftModel = model;
        leftModel = glm::translate(leftModel, glm::vec3(clapOffset, 0.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(leftModel));
        nm = glm::mat3(glm::inverseTranspose(view * leftModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        // during clap or cinematic appear/hold, draw hands without fog so they're in foreground
        bool handsForeground = clapActive || (cinematicActive && (cinematicPhase == 1 || cinematicPhase == 2));
        if (handsForeground && fogEnabledLoc != -1) glUniform1i(fogEnabledLoc, 0);
        LeftHandsModel.Draw(shader, flatFlag);
        if (handsForeground && fogEnabledLoc != -1) glUniform1i(fogEnabledLoc, 1);
    }

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    PlaygroundModel.Draw(shader, flatFlag);

    // Rabbit - apply scaling (appearing from hat)
    {
        glm::mat4 rabbitModel = model;
        rabbitModel = glm::scale(rabbitModel, glm::vec3(rabbitScale, rabbitScale, rabbitScale));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rabbitModel));
        nm = glm::mat3(glm::inverseTranspose(view * rabbitModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        // ensure render mode is applied for the rabbit explicitly
        if (currentRenderMode == RENDER_WIREFRAME) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        // ensure program active and set flat shading uniform specifically for the rabbit draw
        shader.useShaderProgram();
        if (flatShadingLoc != -1) {
            if (currentRenderMode == RENDER_POLYGONAL) glUniform1i(flatShadingLoc, 1);
            else glUniform1i(flatShadingLoc, 0);
        }
        // disable fog while drawing the rabbit so its texture isn't fogged
        if (fogEnabledLoc != -1) glUniform1i(fogEnabledLoc, 0);
        // only draw if scale > 0 (hidden when 0)
        if (rabbitScale > 0.0f) {
            // debug: print render mode and flatShading uniform value
            if (flatShadingLoc != -1) {
                GLint val = -1;
                glGetUniformiv(shader.shaderProgram, flatShadingLoc, &val);
                std::cout << "[Debug] Rabbit renderMode=" << (int)currentRenderMode << " flatShading=" << val << std::endl;
            }
            // enable mesh-level debug prints just for the rabbit draw
            gps::g_debugPrintMeshInfo = true;
            RabbitModel.Draw(shader, flatFlag);
            gps::g_debugPrintMeshInfo = false;
        }
        if (fogEnabledLoc != -1) glUniform1i(fogEnabledLoc, 1);
        // restore polygon mode to current global preference
        if (currentRenderMode == RENDER_WIREFRAME) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Right hand - apply clap translation (mirror)
    {
        glm::mat4 rightModel = model;
        rightModel = glm::translate(rightModel, glm::vec3(-clapOffset, 0.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rightModel));
        nm = glm::mat3(glm::inverseTranspose(view * rightModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        bool handsForegroundR = clapActive || (cinematicActive && (cinematicPhase == 1 || cinematicPhase == 2));
        if (handsForegroundR && fogEnabledLoc != -1) glUniform1i(fogEnabledLoc, 0);
        RightHandsModel.Draw(shader, flatFlag);
        if (handsForegroundR && fogEnabledLoc != -1) glUniform1i(fogEnabledLoc, 1);
    }

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    SceneModel.Draw(shader, flatFlag);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    // Swing: apply small oscillating rotation around its top pivot to simulate gentle swinging
    {
        // choose pivot near top of the model in model space
        glm::vec3 swingPivotModel = SwingModel.getMaxBounds();
        // user-requested: very small forward/back rotation
        float swingAmplitudeDeg = 6.0f; // degrees
        float swingSpeed = 0.8f; // oscillations per second
        float angle = glm::radians(swingAmplitudeDeg) * sin((float)glfwGetTime() * swingSpeed);
        glm::mat4 swingTransform = model * glm::translate(glm::mat4(1.0f), swingPivotModel)
                                    * glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1.0f, 0.0f, 0.0f))
                                    * glm::translate(glm::mat4(1.0f), -swingPivotModel);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(swingTransform));
        nm = glm::mat3(glm::inverseTranspose(view * swingTransform));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        SwingModel.Draw(shader, flatFlag);
        // restore shared model matrix for subsequent draws
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        nm = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    }

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    WheelModel.Draw(shader, flatFlag);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    TreesModel.Draw(shader, flatFlag);
}

void renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//render the scene

    // render all loaded models
    renderModels(myBasicShader);

    // render rain overlay across the whole view
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    rainShader.useShaderProgram();
    if (rainTimeLoc != -1) glUniform1f(rainTimeLoc, (float)glfwGetTime());
    if (rainCamPosLoc != -1) {
        glm::vec3 cp = myCamera.getPosition();
        glUniform3fv(rainCamPosLoc, 1, glm::value_ptr(cp));
    }
    if (rainIntensityLoc != -1) glUniform1f(rainIntensityLoc, rainIntensity);
    if (rainColorLoc != -1) glUniform3fv(rainColorLoc, 1, glm::value_ptr(rainColor));
    glBindVertexArray(rainQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // draw skybox last
    mySkyBox.Draw(skyboxShader, view, projection);

    // camera position logging moved to mouse click handler

}

void updateCinematic(float delta) {
    if (!cinematicActive) return;
    // while cinematic runs, force hands to clap
    clapActive = true;
    cinematicTime += delta;

    // compute hat center world same as in renderModels
    glm::vec3 hatMinModel = HatModel.getMinBounds();
    glm::vec3 hatMaxModel = HatModel.getMaxBounds();
    glm::vec3 hatMinWorld = glm::vec3(model * glm::vec4(hatMinModel, 1.0f));
    glm::vec3 hatMaxWorld = glm::vec3(model * glm::vec4(hatMaxModel, 1.0f));
    glm::vec3 hatCenterWorld = glm::vec3((hatMinWorld + hatMaxWorld) * 0.5f);
    hatCenterWorld.y -= 0.40f;

    // define cinematic positions
    glm::vec3 topPos = hatCenterWorld + glm::vec3(0.0f, 12.0f, 12.0f);
    glm::vec3 nearPos = hatCenterWorld + glm::vec3(0.0f, 2.5f, 6.0f);

    if (cinematicPhase == 0) {
        // descend
        float t = glm::clamp(cinematicTime / CIN_DESCEND, 0.0f, 1.0f);
        glm::vec3 pos = glm::mix(topPos, nearPos, t);
        myCamera.setPosition(pos);
        myCamera.setTarget(hatCenterWorld);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        if (t >= 1.0f) {
            cinematicPhase = 1;
            cinematicTime = 0.0f;
            // start rabbit appear + clap
            clapActive = true;
        }
    } else if (cinematicPhase == 1) {
        // rabbit appear while clapping
        float t = glm::clamp(cinematicTime / CIN_RABBIT, 0.0f, 1.0f);
        rabbitScale = t; // animate from 0 to 1
        // keep camera fixed at nearPos
        myCamera.setPosition(nearPos);
        myCamera.setTarget(hatCenterWorld);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        if (t >= 1.0f) {
            // transition to hands-focus phase
            cinematicPhase = 2;
            cinematicTime = 0.0f;
            clapActive = true; // ensure clapping
            cinematicHandsStartPos = myCamera.getPosition();
        }
    } else if (cinematicPhase == 2) {
        // hands focus: move camera to show hands and keep them clapping for CIN_HANDS_FOCUS
        glm::vec3 leftCenterModel = LeftHandsModel.getCenter();
        glm::vec3 rightCenterModel = RightHandsModel.getCenter();
        glm::vec3 leftWorld = glm::vec3(model * glm::vec4(leftCenterModel, 1.0f));
        glm::vec3 rightWorld = glm::vec3(model * glm::vec4(rightCenterModel, 1.0f));
        glm::vec3 handsCenter = (leftWorld + rightWorld) * 0.5f;
        glm::vec3 desiredHandsPos = handsCenter + glm::vec3(0.0f, 2.0f, 4.5f);
        float moveT = glm::clamp(cinematicTime / CIN_HANDS_MOVE, 0.0f, 1.0f);
        glm::vec3 pos = glm::mix(cinematicHandsStartPos, desiredHandsPos, moveT);
        myCamera.setPosition(pos);
        myCamera.setTarget(handsCenter);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // keep clapping while focused on hands; once camera move completes, continue
        clapActive = true;
        if (moveT >= 1.0f) {
            // after reaching hands position, stop clap, hide rabbit, and start exploration at Wheel
            clapActive = false;
            rabbitScale = 0.0f;
            cinematicPhase = 3;
            cinematicTime = 0.0f;
            cinematicExploreIndex = 0;
            cinematicExploreStartPos = myCamera.getPosition();
        }
    } else if (cinematicPhase == 3) {
        // exploration: visit Wheel, IceCream, Swing focusing on each
        // compute centers
        glm::vec3 wheelCenterModel = WheelModel.getCenter();
        glm::vec3 iceCenterModel = IceCreamModel.getCenter();
        glm::vec3 swingCenterModel = SwingModel.getCenter();
        glm::vec3 wheelCenterWorld = glm::vec3(model * glm::vec4(wheelCenterModel, 1.0f));
        glm::vec3 iceCenterWorld = glm::vec3(model * glm::vec4(iceCenterModel, 1.0f));
        glm::vec3 swingCenterWorld = glm::vec3(model * glm::vec4(swingCenterModel, 1.0f));

        // desired camera offsets relative to each focus center
        glm::vec3 offsets[3] = {
            glm::vec3(0.0f, 4.0f, 8.0f),   // Wheel: in front
            glm::vec3(8.0f, 3.5f, 0.0f),   // IceCream: from right
            glm::vec3(-8.0f, 3.5f, 0.0f)   // Swing: from left
        };
        glm::vec3 targets[3] = { wheelCenterWorld, iceCenterWorld, swingCenterWorld };
        int nTargets = 3;

        // clamp index
        if (cinematicExploreIndex < 0) cinematicExploreIndex = 0;
        if (cinematicExploreIndex >= nTargets) {
            // finished exploration
            cinematicPhase = 4;
            cinematicTime = 0.0f;
        } else {
            // compute desired position for current focus
            glm::vec3 desiredPos = targets[cinematicExploreIndex] + offsets[cinematicExploreIndex];
            float t = glm::clamp(cinematicTime / CIN_EXPLORE_PER, 0.0f, 1.0f);
            glm::vec3 pos = glm::mix(cinematicExploreStartPos, desiredPos, t);
            myCamera.setPosition(pos);
            myCamera.setTarget(targets[cinematicExploreIndex]);
            view = myCamera.getViewMatrix();
            myBasicShader.useShaderProgram();
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

            if (t >= 1.0f) {
                // advance to next focus
                cinematicExploreIndex++;
                cinematicTime = 0.0f;
                cinematicExploreStartPos = myCamera.getPosition();
            }
        }
    } else if (cinematicPhase == 4) {
        // return to saved camera position smoothly
        float t = glm::clamp(cinematicTime / CIN_RETURN, 0.0f, 1.0f);
        glm::vec3 pos = glm::mix(myCamera.getPosition(), cinematic_savedPos, t);
        glm::vec3 target = glm::mix(hatCenterWorld, cinematic_savedTarget, t);
        myCamera.setPosition(pos);
        myCamera.setTarget(target);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        if (t >= 1.0f) {
            cinematicPhase = 5;
            cinematicActive = false;
            cinematicTime = 0.0f;
            // ensure rabbit hidden and clap off
            rabbitScale = 0.0f;
            clapActive = false;
        }
    }
}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
	initModels();
    initSkybox();
    initShaders();
    initRain();
    // clamp camera maximum height to prevent flying above trees
    myCamera.setMaxHeight(18.518449f);
    // restrict camera movement to specified bounds
    myCamera.setMovementBounds(
        glm::vec3(-16.6564f, 1.5543f, -20.506f),
        glm::vec3(27.2437f, 18.518449f, 19.3505f)
    );
	initUniforms();
    setWindowCallbacks();

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
    // update cinematic (if active) and prevent manual movement while it runs
    static double lastFrameTime = glfwGetTime();
    double currentFrame = glfwGetTime();
    float delta = (float)(currentFrame - lastFrameTime);
    lastFrameTime = currentFrame;
    updateCinematic(delta);

    processMovement();
    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
