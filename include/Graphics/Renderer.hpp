#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <Math/Vector3.hpp>
namespace Graphics {
    class Renderer {
        private:
            GLFWwindow* window;
            int width;
            int height;

            // GPU resource management variables
            unsigned int shaderProgram;
            unsigned int VAO, VBO;

            // Local init vars
            void initShaders();
            void initSphere(int sectorCount, int stackCount);

            // A variable storing the number of triangles so the draw function knows how many to draw
            unsigned int indexCount;

            // Camera system
            glm::vec3 cameraPos;
            glm::vec3 cameraFront;
            glm::vec3 cameraUp;

            float yaw; // Left/right rotation angle
            float pitch; // Tilt angle
            float lastX; // Mouse X-coordinate in the previous frame
            float lastY; // Mouse Y-coordinate in the previous frame
            bool firstMouse; // First mouse-over check

            // Post-Processing and FBO
            unsigned int FBO;
            unsigned int textureColorbuffer;
            unsigned int RBO; // Renderbuffer for Depth

            unsigned int quadVAO, quadVBO;
            unsigned int screenShaderProgram;

            void initFBO();

            // Gaussian Blur
            unsigned int pingpongFBO[2];
            unsigned int pingpongColorbuffers[2];
            unsigned int blurShaderProgram;
        public:
            Renderer(int w, int h, const char* title);

            // Destructor
            ~Renderer();

            bool shouldClose() const;
            void clear() const;
            void swapBuffers() const;
            void pollEvents() const;

            void draw(size_t count, const std::vector<Vector3>& positions, const std::vector<double>& masses) const;
            void processInput(float deltaTime); // Function to read keyboard and mouse input for movement

            void beginUI() const; // Start drawing the interface for the frame
            void endUI() const; // Push the interface to the display GPU

            // Get camera data for UI
            glm::vec3 getCameraPos() const {return cameraPos;}
            glm::vec3 getCameraFront() const {return cameraFront;}

            // Declare a function to expose the window to main.cpp for reading the ALT key
            GLFWwindow* getWindow() const {return window;}

            // Declare a function to calculate the ray direction
            glm::vec3 getRayDirection(float mouseX, float mouseY) const;

            unsigned int getBlurredTexture() const { return pingpongColorbuffers[0]; }
    };
}