#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <Math/Vector3.hpp>
#include <Simulation/Simulation.hpp>

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

            int lockedTargetIndex; // The index of locked planet (-1 is free-flight mode)
            float orbitDistance; // Distance from the camera to the center of the planet
            float orbitTheta; // Horizontal rotation angle (yaw) during orbiting
            float orbitPhi; // Vertical pitch angle during orbit

            // Post-Processing and FBO
            unsigned int FBO;
            unsigned int textureColorbuffer; // Ordinary scenery
            unsigned int textureBloombuffer; // Contains only the bloom area
            unsigned int RBO; // Renderbuffer for Depth

            unsigned int quadVAO, quadVBO;
            unsigned int screenShaderProgram;

            void initFBO();

            // Gaussian Blur for UI blur
            unsigned int pingpongFBO[2];
            unsigned int pingpongColorbuffers[2];

            // Gaussian blur for bloom
            unsigned int pingpongFBO_Bloom[2];
            unsigned int pingpongColorbuffers_Bloom[2];

            unsigned int blurShaderProgram;

            // Skybox shader
            unsigned int skyboxShaderProgram;
        public:
            Renderer(int w, int h, const char* title);

            // Destructor
            ~Renderer();

            bool shouldClose() const;
            void clear() const;
            void swapBuffers() const;
            void pollEvents() const;

            void draw(size_t count,
                      const std::vector<Vector3>& positions,
                      const std::vector<double>& radii,
                      const std::vector<glm::quat>& orientations,
                      const std::vector<Simulation::BodyType>& types,
                      const std::vector<double>& temperatures) const;

            void processInput(float deltaTime); // Function to read keyboard and mouse input for movement

            void beginUI() const; // Start drawing the interface for the frame
            void endUI() const; // Push the interface to the display GPU

            // Get camera data for UI
            glm::vec3 getCameraPos() const {return cameraPos;}
            glm::vec3 getCameraFront() const {return cameraFront;}

            // API for Camera Tracking
            float cameraBaseSpeed;
            void lockTarget(int entityIndex, float distance = 50.0f);
            void unlockTarget();
            bool isTargetLocked() const { return lockedTargetIndex != -1; }
            int getLockedTargetIndex() const { return lockedTargetIndex; }
            void updateCameraTracking(const std::vector<Vector3>& positions);

            // Declare a function to expose the window to main.cpp for reading the ALT key
            GLFWwindow* getWindow() const {return window;}

            // Declare a function to calculate the ray direction
            glm::vec3 getRayDirection(float mouseX, float mouseY) const;

            unsigned int getBlurredTexture() const { return pingpongColorbuffers[0]; }
    };
}
