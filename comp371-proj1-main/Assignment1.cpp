//
// COMP 371 Lab 6 - Models and EBOs
//
// Created by Zachary Lapointe on 17/07/2019.
//

#include <iostream>
#include <algorithm>
#include <vector>
#include <filesystem>

#define GLEW_STATIC 1 // This allows linking with Static Library on Windows, without DLL
#include <GL/glew.h>  // Include GLEW - OpenGL Extension Wrangler

#include <GLFW/glfw3.h> // GLFW provides a cross-platform interface for creating a graphical context,
                        // initializing OpenGL and binding inputs

#include <glm/glm.hpp>                  // GLM is an optimized math library with syntax to similar to OpenGL Shading Language
#include <glm/gtc/matrix_transform.hpp> // include this to create transformation matrices
#include <glm/common.hpp>

#include "OBJloader.h"   //For loading .obj files
#include "OBJloaderV2.h" //For loading .obj files using a polygon list format

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

using namespace glm;
using namespace std;

GLuint loadTexture(const char *filename);

const char *getVertexShaderSource()
{
    // For now, you use a string for your shader code, in the assignment, shaders will be stored in .glsl files
    return "#version 330 core\n"
           "layout (location = 0) in vec3 aPos;\n"
           "layout (location = 1) in vec3 aNormal;\n"
           "layout (location = 2) in vec2 aTexCoord;\n"
           "\n"
           "out vec2 TexCoord;\n"
           "\n"
           "uniform mat4 worldMatrix;\n"
           "uniform mat4 viewMatrix;\n"
           "uniform mat4 projectionMatrix;\n"
           "\n"
           "   void main()\n"
           "   {\n"
           "       gl_Position = projectionMatrix * viewMatrix * worldMatrix * vec4(aPos, 1.0);\n"
           "       TexCoord = aTexCoord;\n"
           "   }\n";
}

const char *getFragmentShaderSource()
{
    return "#version 330 core\n"
           "in vec2 TexCoord;\n"
           "out vec4 FragColor;\n"
           "\n"
           "uniform sampler2D planetTexture;\n"
           "\n"
           "   void main()\n"
           "   {\n"
           "       FragColor = texture(planetTexture, TexCoord);\n"
           "   }\n";
}

int compileAndLinkShaders(const char *vertexShaderSource, const char *fragmentShaderSource)
{
    // compile and link shader program
    // return shader program id
    // ------------------------------------

    // vertex shader
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    // fragment shader
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    // link shaders
    int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void setProjectionMatrix(int shaderProgram, mat4 projectionMatrix)
{
    glUseProgram(shaderProgram);
    GLuint projectionMatrixLocation = glGetUniformLocation(shaderProgram, "projectionMatrix");
    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
}

void setViewMatrix(int shaderProgram, mat4 viewMatrix)
{
    glUseProgram(shaderProgram);
    GLuint viewMatrixLocation = glGetUniformLocation(shaderProgram, "viewMatrix");
    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
}

void setWorldMatrix(int shaderProgram, mat4 worldMatrix)
{
    glUseProgram(shaderProgram);
    GLuint worldMatrixLocation = glGetUniformLocation(shaderProgram, "worldMatrix");
    glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &worldMatrix[0][0]);
}

GLuint setupModelVBO(string path, int &vertexCount)
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> UVs;

    //read the vertex data from the model's OBJ file
    loadOBJ(path.c_str(), vertices, normals, UVs);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO); //Becomes active VAO
    // Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).

    //Vertex VBO setup
    GLuint vertices_VBO;
    glGenBuffers(1, &vertices_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, vertices_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    //Normals VBO setup
    GLuint normals_VBO;
    glGenBuffers(1, &normals_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, normals_VBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(1);

    //UVs VBO setup
    GLuint uvs_VBO;
    glGenBuffers(1, &uvs_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, uvs_VBO);
    glBufferData(GL_ARRAY_BUFFER, UVs.size() * sizeof(glm::vec2), &UVs.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(2);

    glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs, as we are using multiple VAOs)
    vertexCount = vertices.size();
    return VAO;
}

//Sets up a model using an Element Buffer Object to refer to vertex data
GLuint setupModelEBO(string path, int &vertexCount)
{
    vector<int> vertexIndices; //The contiguous sets of three indices of vertices, normals and UVs, used to make a triangle
    vector<glm::vec3> vertices;
    vector<glm::vec3> normals;
    vector<glm::vec2> UVs;

    //read the vertices from the cube.obj file
    //We won't be needing the normals or UVs for this program
    loadOBJ2(path.c_str(), vertexIndices, vertices, normals, UVs);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO); //Becomes active VAO
    // Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).

    //Vertex VBO setup
    GLuint vertices_VBO;
    glGenBuffers(1, &vertices_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, vertices_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    //Normals VBO setup
    GLuint normals_VBO;
    glGenBuffers(1, &normals_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, normals_VBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(1);

    //UVs VBO setup
    GLuint uvs_VBO;
    glGenBuffers(1, &uvs_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, uvs_VBO);
    glBufferData(GL_ARRAY_BUFFER, UVs.size() * sizeof(glm::vec2), &UVs.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(2);

    //EBO setup
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIndices.size() * sizeof(int), &vertexIndices.front(), GL_STATIC_DRAW);

    glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO
    vertexCount = vertexIndices.size();
    return VAO;
}

int main(int argc, char *argv[])
{
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
    // Initialize GLFW and OpenGL version
    glfwInit();

#ifdef __APPLE__
#ifndef PLATFORM_OSX
#define PLATFORM_OSX
#endif
#endif

#if defined(PLATFORM_OSX)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    // On windows, we set OpenGL version to 2.1, to support more hardware
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#endif

    // Create Window and rendering context using GLFW, resolution is 800x600
    GLFWwindow *window = glfwCreateWindow(800, 600, "Comp371 - Solar System", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to create GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }

    GLuint sunTextureID = loadTexture("Textures/sun.jpg");
    GLuint mercuryTextureID = loadTexture("Textures/mercury.jpg");
    GLuint venusTextureID = loadTexture("Textures/venus.jpg");
    GLuint earthTextureID = loadTexture("Textures/earth.jpg");
    GLuint marsTextureID = loadTexture("Textures/mars.jpg");
    GLuint jupiterTextureID = loadTexture("Textures/jupiter.jpg");
    GLuint saturnTextureID = loadTexture("Textures/saturn.jpg");
    GLuint uranusTextureID = loadTexture("Textures/uranus.jpg");
    GLuint neptuneTextureID = loadTexture("Textures/neptune.jpg");
    GLuint moonTextureID = loadTexture("Textures/moon.jpg");

    // Black background
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Compile and link shaders here ...
    int whiteShaderProgram = compileAndLinkShaders(getVertexShaderSource(), getFragmentShaderSource());

    //Setup models
    string planetPath = "Models/sphere.obj";

    int sunVertices;
    GLuint sunVAO = setupModelEBO(planetPath, sunVertices);

    int mercuryVertices;
    GLuint mercuryVAO = setupModelEBO(planetPath, mercuryVertices);

    int moonVertices;
    GLuint moonVAO = setupModelEBO(planetPath, moonVertices);

    int venusVertices;
    GLuint venusVAO = setupModelEBO(planetPath, venusVertices);

    int earthVertices;
    GLuint earthVAO = setupModelEBO(planetPath, earthVertices);

    int marsVertices;
    GLuint marsVAO = setupModelEBO(planetPath, marsVertices);

    int jupiterVertices;
    GLuint jupiterVAO = setupModelEBO(planetPath, jupiterVertices);

    int saturnVertices;
    GLuint saturnVAO = setupModelEBO(planetPath, saturnVertices);

    int uranusVertices;
    GLuint uranusVAO = setupModelEBO(planetPath, uranusVertices);

    int neptuneVertices;
    GLuint neptuneVAO = setupModelEBO(planetPath, neptuneVertices);

    // Camera parameters for view transform
    vec3 cameraPosition(15.0f, 1.0f, 30.0f);
    vec3 cameraLookAt(0.0f, 0.0f, -1.0f);
    vec3 cameraUp(0.0f, 1.0f, 0.0f);

    // Other camera parameters
    float cameraSpeed = 1.0f;
    float cameraFastSpeed = 4 * cameraSpeed;
    float cameraHorizontalAngle = 90.0f;
    float cameraVerticalAngle = 0.0f;

    // Spinning cube at camera position
    float spinningAngle = 0.0f;

    //*****Orbital angles for each planet
    float orbitalAngleMercury = 0.0f;
    float orbitalAngleVenus = 0.0f;
    float orbitalAngleEarth = 0.0f;
    float orbitalAngleMars = 0.0f;
    float orbitalAngleJupiter = 0.0f;
    float orbitalAngleSaturn = 0.0f;
    float orbitalAngleUranus = 0.0f;
    float orbitalAngleNeptune = 0.0f;
    float orbitalAngleMoon = 0.0f;

    //******Orbital Speeds (rad per second)*********
    const float orbitalSpeedMercury = glm::radians(50.0f);
    const float orbitalSpeedVenus = glm::radians(35.0f);
    const float orbitalSpeedEarth = glm::radians(30.0f);
    const float orbitalSpeedMars = glm::radians(24.0f);
    const float orbitalSpeedJupiter = glm::radians(13.0f);
    const float orbitalSpeedSaturn = glm::radians(9.0f);
    const float orbitalSpeedUranus = glm::radians(6.0f);
    const float orbitalSpeedNeptune = glm::radians(5.0f);
    const float orbitalSpeedMoon = glm::radians(100.0f);

    // Set projection matrix for shader, this won't change
    mat4 projectionMatrix = glm::perspective(70.0f,           // field of view in degrees
                                             800.0f / 600.0f, // aspect ratio
                                             0.01f, 100.0f);  // near and far (near > 0)

    // Set initial view matrix
    mat4 viewMatrix = lookAt(cameraPosition,                // eye
                             cameraPosition + cameraLookAt, // center
                             cameraUp);                     // up

    // Set View and Projection matrices on both shaders
    setViewMatrix(whiteShaderProgram, viewMatrix);

    setProjectionMatrix(whiteShaderProgram, projectionMatrix);

    // For frame time
    float lastFrameTime = glfwGetTime();
    int lastMouseLeftState = GLFW_RELEASE;
    double lastMousePosX, lastMousePosY;
    glfwGetCursorPos(window, &lastMousePosX, &lastMousePosY);

    // Other OpenGL states to set once
    // Enable Backface culling
    glEnable(GL_DEPTH_TEST);

    // Entering Main Loop
    while (!glfwWindowShouldClose(window))
    {
        // Frame time calculation
        float dt = glfwGetTime() - lastFrameTime;
        lastFrameTime += dt;

        // Spinning model rotation animation
        spinningAngle += 45.0f * dt; //This is equivalent to 45 degrees per second

        //************Update Angles
        orbitalAngleMercury += orbitalSpeedMercury * dt;
        orbitalAngleVenus += orbitalSpeedVenus * dt;
        orbitalAngleEarth += orbitalSpeedEarth * dt;
        orbitalAngleMars += orbitalSpeedMars * dt;
        orbitalAngleJupiter += orbitalSpeedJupiter * dt;
        orbitalAngleSaturn += orbitalSpeedSaturn * dt;
        orbitalAngleUranus += orbitalSpeedUranus * dt;
        orbitalAngleNeptune += orbitalSpeedNeptune * dt;
        orbitalAngleMoon += orbitalSpeedMoon * dt;

        // Each frame, reset color of each pixel to glClearColor
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw colored geometry
        glUseProgram(whiteShaderProgram);

        // Set the view matrix for first person camera
        mat4 viewMatrix(1.0f);
        viewMatrix = lookAt(cameraPosition, cameraPosition + cameraLookAt, cameraUp);
        setViewMatrix(whiteShaderProgram, viewMatrix);

        // Set sun world matrix
        mat4 sunWorldMatrix =
            glm::translate(mat4(1.0f), vec3(0.0f, 0.0f, 0.0f)) *
            glm::rotate(mat4(1.0f), radians(spinningAngle), vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(mat4(1.0f), radians(-90.0f), vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(mat4(1.0f), vec3(0.2f));
        setWorldMatrix(whiteShaderProgram, sunWorldMatrix);

        // Bind sun texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sunTextureID);
        glUniform1i(glGetUniformLocation(whiteShaderProgram, "planetTexture"), 0);
        glBindVertexArray(sunVAO);
        glDrawElements(GL_TRIANGLES, sunVertices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Set mercury world matrix
        mat4 mercuryPositionMatrix =
            glm::rotate(mat4(1.0f), orbitalAngleMercury, vec3(0.0f, 1.0f, 0.0f)) *
            glm::translate(mat4(1.0f), vec3(4.0f, 0.0f, 0.0f));

        mat4 mercuryWorldMatrix =
            mercuryPositionMatrix *
            glm::rotate(mat4(1.0f), radians(spinningAngle), vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(mat4(1.0f), radians(-90.0f), vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(mat4(1.0f), vec3(0.03f));
        setWorldMatrix(whiteShaderProgram, mercuryWorldMatrix);

        // Bind mercury texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mercuryTextureID);
        glUniform1i(glGetUniformLocation(whiteShaderProgram, "planetTexture"), 0);
        glBindVertexArray(mercuryVAO);
        glDrawElements(GL_TRIANGLES, mercuryVertices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Set venus world matrix
        mat4 venusPositionMatrix =
            glm::rotate(mat4(1.0f), orbitalAngleVenus, vec3(0.0f, 1.0f, 0.0f)) *
            glm::translate(mat4(1.0f), vec3(7.0f, 0.0f, 0.0f));
        mat4 venusWorldMatrix =
            venusPositionMatrix *
            glm::rotate(mat4(1.0f), radians(spinningAngle), vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(mat4(1.0f), radians(-90.0f), vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(mat4(1.0f), vec3(0.05f));
        setWorldMatrix(whiteShaderProgram, venusWorldMatrix);

        // Bind venus texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, venusTextureID);
        glUniform1i(glGetUniformLocation(whiteShaderProgram, "planetTexture"), 0);
        glBindVertexArray(venusVAO);
        glDrawElements(GL_TRIANGLES, venusVertices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Set earth world matrix

        mat4 earthPositionMatrix =
            glm::rotate(mat4(1.0f), orbitalAngleEarth, vec3(0.0f, 1.0f, 0.0f)) *
            glm::translate(mat4(1.0f), vec3(10.0f, 0.0f, 0.0f));
        mat4 earthWorldMatrix =
            earthPositionMatrix *
            glm::rotate(mat4(1.0f), radians(spinningAngle), vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(mat4(1.0f), radians(-90.0f), vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(mat4(1.0f), vec3(0.05f));
        setWorldMatrix(whiteShaderProgram, earthWorldMatrix);

        // Bind earth texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, earthTextureID);
        glUniform1i(glGetUniformLocation(whiteShaderProgram, "planetTexture"), 0);
        glBindVertexArray(earthVAO);
        glDrawElements(GL_TRIANGLES, earthVertices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        //*********  Moon  *********
        mat4 moonPositionMatrix =
            earthPositionMatrix *
            glm::rotate(mat4(1.0f), orbitalAngleMoon, vec3(0.0f, 1.0f, 0.0f)) *
            glm::translate(mat4(1.0f), vec3(0.5f, 0.0f, 0.0f));
        mat4 moonWorldMatrix =
            moonPositionMatrix *
            glm::rotate(mat4(1.0f), radians(spinningAngle), vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(mat4(1.0f), radians(-90.0f), vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(mat4(1.0f), vec3(0.015f));
        setWorldMatrix(whiteShaderProgram, moonWorldMatrix);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, moonTextureID);
        glUniform1i(glGetUniformLocation(whiteShaderProgram, "planetTexture"), 0);
        glBindVertexArray(moonVAO);
        glDrawElements(GL_TRIANGLES, moonVertices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Set mars world matrix
        mat4 marsPositionMatrix =
            glm::rotate(mat4(1.0f), orbitalAngleMars, vec3(0.0f, 1.0f, 0.0f)) *
            glm::translate(mat4(1.0f), vec3(12.0f, 0.0f, 0.0f));
        mat4 marsWorldMatrix =
            marsPositionMatrix *
            glm::rotate(mat4(1.0f), radians(spinningAngle), vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(mat4(1.0f), radians(-90.0f), vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(mat4(1.0f), vec3(0.04f));
        setWorldMatrix(whiteShaderProgram, marsWorldMatrix);

        // Bind mars texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, marsTextureID);
        glUniform1i(glGetUniformLocation(whiteShaderProgram, "planetTexture"), 0);
        glBindVertexArray(marsVAO);
        glDrawElements(GL_TRIANGLES, marsVertices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Set jupiter world matrix
        mat4 jupiterPositionMatrix =
            glm::rotate(mat4(1.0f), orbitalAngleJupiter, vec3(0.0f, 1.0f, 0.0f)) *
            glm::translate(mat4(1.0f), vec3(15.0f, 0.0f, 0.0f));
        mat4 jupiterWorldMatrix =
            jupiterPositionMatrix *
            glm::rotate(mat4(1.0f), radians(spinningAngle), vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(mat4(1.0f), radians(-90.0f), vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(mat4(1.0f), vec3(0.15f));
        setWorldMatrix(whiteShaderProgram, jupiterWorldMatrix);

        // Bind jupiter texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, jupiterTextureID);
        glUniform1i(glGetUniformLocation(whiteShaderProgram, "planetTexture"), 0);
        glBindVertexArray(jupiterVAO);
        glDrawElements(GL_TRIANGLES, jupiterVertices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Set saturn world matrix
        mat4 saturnPositionMatrix =
            glm::rotate(mat4(1.0f), orbitalAngleSaturn, vec3(0.0f, 1.0f, 0.0f)) *
            glm::translate(mat4(1.0f), vec3(19.0f, 0.0f, 0.0f));
        mat4 saturnWorldMatrix =
            saturnPositionMatrix *
            glm::rotate(mat4(1.0f), radians(spinningAngle), vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(mat4(1.0f), radians(-90.0f), vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(mat4(1.0f), vec3(0.14f));
        setWorldMatrix(whiteShaderProgram, saturnWorldMatrix);

        // Bind saturn texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, saturnTextureID);
        glUniform1i(glGetUniformLocation(whiteShaderProgram, "planetTexture"), 0);
        glBindVertexArray(saturnVAO);
        glDrawElements(GL_TRIANGLES, saturnVertices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Set uranus world matrix
        mat4 uranusPositionMatrix =
            glm::rotate(mat4(1.0f), orbitalAngleUranus, vec3(0.0f, 1.0f, 0.0f)) *
            glm::translate(mat4(1.0f), vec3(23.0f, 0.0f, 0.0f));
        mat4 uranusWorldMatrix =
            uranusPositionMatrix *
            glm::rotate(mat4(1.0f), radians(spinningAngle), vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(mat4(1.0f), radians(-90.0f), vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(mat4(1.0f), vec3(0.07f));
        setWorldMatrix(whiteShaderProgram, uranusWorldMatrix);

        // Bind uranus texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, uranusTextureID);
        glUniform1i(glGetUniformLocation(whiteShaderProgram, "planetTexture"), 0);
        glBindVertexArray(uranusVAO);
        glDrawElements(GL_TRIANGLES, uranusVertices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Set neptune world matrix
        mat4 neptunePositionMatrix =
            glm::rotate(mat4(1.0f), orbitalAngleNeptune, vec3(0.0f, 1.0f, 0.0f)) *
            glm::translate(mat4(1.0f), vec3(26.0f, 0.0f, 0.0f));
        mat4 neptuneWorldMatrix =
            neptunePositionMatrix *
            glm::rotate(mat4(1.0f), radians(spinningAngle), vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(mat4(1.0f), radians(-90.0f), vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(mat4(1.0f), vec3(0.07f));
        setWorldMatrix(whiteShaderProgram, neptuneWorldMatrix);

        // Bind neptune texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, neptuneTextureID);
        glUniform1i(glGetUniformLocation(whiteShaderProgram, "planetTexture"), 0);
        glBindVertexArray(neptuneVAO);
        glDrawElements(GL_TRIANGLES, neptuneVertices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // End Frame
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Handle inputs
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // This was solution for Lab02 - Moving camera exercise
        // We'll change this to be a first or third person camera
        bool fastCam = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        float currentCameraSpeed = (fastCam) ? cameraFastSpeed : cameraSpeed;

        // - Calculate mouse motion dx and dy
        // - Update camera horizontal and vertical angle
        double mousePosX, mousePosY;
        glfwGetCursorPos(window, &mousePosX, &mousePosY);

        double dx = mousePosX - lastMousePosX;
        double dy = mousePosY - lastMousePosY;

        lastMousePosX = mousePosX;
        lastMousePosY = mousePosY;

        // Convert to spherical coordinates
        const float cameraAngularSpeed = 15.0f;
        cameraHorizontalAngle -= dx * cameraAngularSpeed * dt;
        cameraVerticalAngle -= dy * cameraAngularSpeed * dt;

        // Clamp vertical angle to [-85, 85] degrees
        cameraVerticalAngle = std::max(-85.0f, std::min(85.0f, cameraVerticalAngle));

        float theta = radians(cameraHorizontalAngle);
        float phi = radians(cameraVerticalAngle);

        cameraLookAt = vec3(cosf(phi) * cosf(theta), sinf(phi), -cosf(phi) * sinf(theta));
        vec3 cameraSideVector = glm::cross(cameraLookAt, vec3(0.0f, 1.0f, 0.0f));

        glm::normalize(cameraSideVector);

        // Use camera lookat and side vectors to update positions with ASDW
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            cameraPosition += cameraLookAt * dt * currentCameraSpeed;
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            cameraPosition -= cameraLookAt * dt * currentCameraSpeed;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            cameraPosition += cameraSideVector * dt * currentCameraSpeed;
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            cameraPosition -= cameraSideVector * dt * currentCameraSpeed;
        }
    }

    glfwTerminate();

    return 0;
}

GLuint loadTexture(const char *filename)
{
    stbi_set_flip_vertically_on_load(true); // optional, depending on your texture orientation

    int width, height, nrChannels;
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);

    if (!data)
    {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture filtering/wrapping options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureID;
}