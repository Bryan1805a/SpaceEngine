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
            void renderUI(size_t bodyCount); // Draw the control panels
            void endUI() const; // Push the interface to the display GPU
    };
}