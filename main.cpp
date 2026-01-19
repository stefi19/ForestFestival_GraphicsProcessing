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

// rabbit-from-hat animation state (instant toggle: 0 or 1)
float rabbitScale = 1.0f;

GLboolean pressedKeys[1024];

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
// skybox
gps::SkyBox mySkyBox;
gps::Shader skyboxShader;

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
            if (key == GLFW_KEY_P) { // toggle clap animation
                clapActive = !clapActive;
                if (!clapActive) { clapOffset = 0.0f; clapDirection = 1; } // reset when turned off
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
    // store current fog params for debugging
    currentFogDensity = fogDensity;
    currentFogRadius = fogRadius;
    currentFogStretchDown = fogStretchDown;
}

void renderModels(gps::Shader shader) {
    shader.useShaderProgram();

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
    // update animated fog time uniform
    if (fogTimeLoc != -1) {
        float t = (float)glfwGetTime();
        glUniform1f(fogTimeLoc, t);
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
    FerisWheelModel.Draw(shader);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    // disable fog when drawing the hat itself so texture isn't fogged
    if (fogEnabledLoc != -1) glUniform1i(fogEnabledLoc, 0);
    HatModel.Draw(shader);
    if (fogEnabledLoc != -1) glUniform1i(fogEnabledLoc, 1);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    IceCreamModel.Draw(shader);

    // Left hand - apply clap translation
    {
        glm::mat4 leftModel = model;
        leftModel = glm::translate(leftModel, glm::vec3(clapOffset, 0.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(leftModel));
        nm = glm::mat3(glm::inverseTranspose(view * leftModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        LeftHandsModel.Draw(shader);
    }

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    PlaygroundModel.Draw(shader);

    // Rabbit - apply scaling (appearing from hat)
    {
        glm::mat4 rabbitModel = model;
        rabbitModel = glm::scale(rabbitModel, glm::vec3(rabbitScale, rabbitScale, rabbitScale));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rabbitModel));
        nm = glm::mat3(glm::inverseTranspose(view * rabbitModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        // disable fog while drawing the rabbit so its texture isn't fogged
        if (fogEnabledLoc != -1) glUniform1i(fogEnabledLoc, 0);
        // only draw if scale > 0 (hidden when 0)
        if (rabbitScale > 0.0f) RabbitModel.Draw(shader);
        if (fogEnabledLoc != -1) glUniform1i(fogEnabledLoc, 1);
    }

    // Right hand - apply clap translation (mirror)
    {
        glm::mat4 rightModel = model;
        rightModel = glm::translate(rightModel, glm::vec3(-clapOffset, 0.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rightModel));
        nm = glm::mat3(glm::inverseTranspose(view * rightModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        RightHandsModel.Draw(shader);
    }

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    SceneModel.Draw(shader);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    SwingModel.Draw(shader);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    WheelModel.Draw(shader);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    nm = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
    TreesModel.Draw(shader);
}

void renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//render the scene

    // render all loaded models
    renderModels(myBasicShader);

    // draw skybox last
    mySkyBox.Draw(skyboxShader, view, projection);

    // camera position logging moved to mouse click handler

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
        processMovement();
	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
