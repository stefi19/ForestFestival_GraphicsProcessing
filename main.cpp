#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> // Core GLM types and operations
#include <glm/gtc/matrix_transform.hpp> // Common matrix transform utilities
#include <glm/gtc/matrix_inverse.hpp> // Matrix inverse and transpose utilities
#include <glm/gtc/type_ptr.hpp> // Utility to access GLM data pointers

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>
#include <cmath>

// Application window instance
gps::Window myWindow;

// Global transformation matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
// Placeholder for unused balloon models
glm::mat3 normalMatrix;
// Static base model matrices for non-animated meshes
glm::mat4 structureBaseModel = glm::mat4(1.0f);
glm::mat4 wheelBaseModel = glm::mat4(1.0f);

// Global light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// Locations of shader uniform variables
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

// Primary scene camera
gps::Camera myCamera(
    glm::vec3(0.0f, 3.0f, 20.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

// Clap animation state
bool clapActive = false;
float clapOffset = 0.0f;
float clapSpeed = 0.015f; // Clap translation speed in units per frame
// Maximum per-hand translation derived from measured palm separation
float clapMax = 0.35f;
// Clap motion direction where 1 is inward and -1 is outward
int clapDirection = 1;

GLfloat cameraSpeed = 0.1f;

// Rabbit visibility scale toggled instantly between zero and one
float rabbitScale = 1.0f;

GLboolean pressedKeys[1024];

// Mouse input state
double lastX = 0.0;
double lastY = 0.0;
bool firstMouse = true;
float mouseSensitivity = 0.1f; // Mouse sensitivity in degrees per pixel

// Scene model instances
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
glm::vec3 wheelPivot = glm::vec3(0.0f);
gps::Model3D TreesModel;
GLfloat angle;
// Wheel rotation state
float wheelAngle = 0.0f;
float wheelSpeed = 0.5f; // Rotation increment in degrees per frame

// Shader programs and skybox
gps::Shader myBasicShader;
gps::SkyBox mySkyBox;
gps::Shader skyboxShader;

GLenum glCheckError_(const char *file, int line)
{
    GLenum errorCode = GL_NO_ERROR;
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
#if 1
    return errorCode;
#endif
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
    // Print new window dimensions on resize
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
            if (key == GLFW_KEY_P) { // Toggle clap animation state
                clapActive = !clapActive;
                if (!clapActive) { clapOffset = 0.0f; clapDirection = 1; } // Reset clap state when deactivated
            }
            if (key == GLFW_KEY_I) { // Toggle rabbit visibility instantly
                if (rabbitScale > 0.0f) {
                    rabbitScale = 0.0f; // Hide rabbit
                } else {
                    rabbitScale = 1.0f; // Show rabbit
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
    double yoffset = lastY - ypos; // Invert Y offset to account for window coordinate origin

    lastX = xpos;
    lastY = ypos;

    float xoff = (float)xoffset * mouseSensitivity;
    float yoff = (float)yoffset * mouseSensitivity;

    // Rotate camera by pitch then yaw
    myCamera.rotate(yoff, xoff);
    // Update shader view uniform
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
        // Update shader view uniform
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
        // Update shader view uniform
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
        // Update shader view uniform
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        // Update shader view uniform
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	}

    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
        // Update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // Update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
        // Update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // Update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }

    // Update clap animation state
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
    // Rabbit visibility is controlled by an instant toggle

    // Advance wheel rotation angle and wrap at 360 degrees
    wheelAngle += wheelSpeed;
    if (wheelAngle >= 360.0f) wheelAngle -= 360.0f;
}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetMouseButtonCallback(myWindow.getWindow(), mouseButtonCallback);
    // Capture and hide cursor for mouse look
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST); // Enable depth testing
    glDepthFunc(GL_LESS); // Use less-only depth comparison
    glEnable(GL_CULL_FACE); // Enable face culling
    glCullFace(GL_BACK); // Cull back faces
    glFrontFace(GL_CCW); // Set counter-clockwise winding as front
}

void initModels() {
    // Teapot model loading disabled
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
    // Compute wheel pivot using the wheel mesh center
    wheelPivot = WheelModel.getCenter();
    std::cout << "Wheel pivot: " << wheelPivot.x << ", " << wheelPivot.y << ", " << wheelPivot.z << std::endl;
    // Preserve original exported mesh placement by keeping identity base transforms
    structureBaseModel = glm::mat4(1.0f);
    wheelBaseModel = glm::mat4(1.0f);
}

void initSkybox()
{
    std::vector<const GLchar*> faces;
    // Skybox face textures ordered as +X -X +Y -Y +Z -Z
    faces.push_back("skybox/posx.jpg");
    faces.push_back("skybox/negx.jpg");
    faces.push_back("skybox/posy.jpg");
    faces.push_back("skybox/negy.jpg");
    faces.push_back("skybox/posz.jpg");
    faces.push_back("skybox/negz.jpg");

    mySkyBox.Load(faces);
}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");

    // Load skybox shader
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    skyboxShader.useShaderProgram();
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    // Initialize model matrix and retrieve uniform location
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

    // Get view matrix and upload to shader
    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Compute normal matrix for current model-view and retrieve uniform location
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

    // Create projection matrix and set uniform
    projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 1000.0f);
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

    // Set global light direction and upload to shader
    lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

    // Set global light color and upload to shader
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
}

void renderModels(gps::Shader shader) {
    shader.useShaderProgram();
    glm::mat3 nm;

    // Ferris wheel structure rendered with static base transform
    {
        glm::mat4 m = structureBaseModel;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        nm = glm::mat3(glm::inverseTranspose(view * m));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        FerisWheelModel.Draw(shader);
    }

    // Hat rendered with identity transform
    {
        glm::mat4 m = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        nm = glm::mat3(glm::inverseTranspose(view * m));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        HatModel.Draw(shader);
    }

    // Ice cream rendered with identity transform
    {
        glm::mat4 m = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        nm = glm::mat3(glm::inverseTranspose(view * m));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        IceCreamModel.Draw(shader);
    }

    // Left hand translated by clap offset
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(clapOffset, 0.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        nm = glm::mat3(glm::inverseTranspose(view * m));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        LeftHandsModel.Draw(shader);
    }

    // Playground rendered with identity transform
    {
        glm::mat4 m = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        nm = glm::mat3(glm::inverseTranspose(view * m));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        PlaygroundModel.Draw(shader);
    }

    // Rabbit rendered with instant scale toggle
    {
        glm::mat4 m = glm::scale(glm::mat4(1.0f), glm::vec3(rabbitScale, rabbitScale, rabbitScale));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        nm = glm::mat3(glm::inverseTranspose(view * m));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        if (rabbitScale > 0.0f) RabbitModel.Draw(shader);
    }

    // Right hand translated by negative clap offset
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(-clapOffset, 0.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        nm = glm::mat3(glm::inverseTranspose(view * m));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        RightHandsModel.Draw(shader);
    }

    // Scene rendered with identity transform
    {
        glm::mat4 m = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        nm = glm::mat3(glm::inverseTranspose(view * m));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        SceneModel.Draw(shader);
    }

    // Swing rendered with identity transform
    {
        glm::mat4 m = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        nm = glm::mat3(glm::inverseTranspose(view * m));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        SwingModel.Draw(shader);
    }

    // Wheel rotated around its local pivot using wheelBaseModel as static base
    {
        glm::mat4 localRotate =
            glm::translate(glm::mat4(1.0f), wheelPivot) *
            glm::rotate(glm::mat4(1.0f), glm::radians(wheelAngle), glm::vec3(1.0f, 0.0f, 0.0f)) *
            glm::translate(glm::mat4(1.0f), -wheelPivot);

        glm::mat4 wheelModel = wheelBaseModel * localRotate;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wheelModel));
        nm = glm::mat3(glm::inverseTranspose(view * wheelModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        WheelModel.Draw(shader);
    }

    // Trees rendered with identity transform
    {
        glm::mat4 m = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        nm = glm::mat3(glm::inverseTranspose(view * m));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));
        TreesModel.Draw(shader);
    }
}

void renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render all scene objects
    renderModels(myBasicShader);

    // Draw skybox last
    mySkyBox.Draw(skyboxShader, view, projection);

    // Camera position is logged on right mouse click

}

void cleanup() {
    myWindow.Delete();
    // Release application resources
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
    // Clamp camera maximum height
    myCamera.setMaxHeight(18.518449f);
    // Restrict camera movement to specified world-space bounds
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
