#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <Math/Vector3.hpp>
#include <Simulation/Simulation.hpp>
#include <Graphics/Shader.hpp>
#include <Graphics/Camera.hpp>
#include <Graphics/Mesh.hpp>
#include <Graphics/PostProcessor.hpp>

namespace Graphics {
    class Renderer {
        private:
            GLFWwindow* window;
            int width;
            int height;

            // GPU resource management variables
            Shader shaderProgram;
            Mesh sphere;

            // Local init vars
            void initShaders();

            // Camera system
            Camera camera;

            // Post-Processing (offscreen FBO + bloom + screen composite)
            PostProcessor postProcessor;

            Mesh quad;
            Shader screenShaderProgram;

            // Gaussian Blur for the UI frosted-glass background
            unsigned int uiPingpongFBO[2];
            unsigned int uiPingpongColorbuffers[2];

            Shader blurShaderProgram;

            // Fullscreen toggle state
            bool isFullscreen = false;
            bool f11WasPressed = false;
            int windowedX = 0, windowedY = 0;
            int windowedWidth = 0, windowedHeight = 0;

            // Persistent UI-cursor toggle state (Tab)
            bool uiCursorEnabled = false;
            bool tabWasPressed = false;

            // Skybox shader
            Shader skyboxShaderProgram;

            // Shadow mapping
            unsigned int depthMapFBO;
            unsigned int depthCubemap;
            Shader shadowShaderProgram;
            const unsigned int SHADOW_RES = 2048;

            unsigned int loadHDRTexture(const char* path);

            // Create the shadow-map FBO and the UI-blur pingpong buffers
            void initFramebuffers();
        public:
            Renderer(int w, int h, const char* title);

            // Destructor
            ~Renderer();

            bool shouldClose() const;
            void clear() const;
            void swapBuffers() const;
            void pollEvents();

            // Re-create FBOs when the window resolution changes
            void resize(int newWidth, int newHeight);

            int getWidth() const {return width;}
            int getHeight() const {return height;}

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
            glm::vec3 getCameraPos() const {return camera.Position;}
            glm::vec3 getCameraFront() const {return camera.Front;}
            float getCameraYaw() const {return camera.Yaw;}
            float getCameraPitch() const {return camera.Pitch;}

            // API for Camera Tracking
            float& getCameraSpeed() {return camera.MovementSpeed;}
            void lockTarget(int entityIndex, float distance = 50.0f);
            void unlockTarget();
            bool isTargetLocked() const { return camera.isTargetLocked(); }
            int getLockedTargetIndex() const { return camera.lockedTargetIndex; }
            void updateCameraTracking(const std::vector<Vector3>& positions);

            // Declare a function to expose the window to main.cpp for reading the ALT key
            GLFWwindow* getWindow() const {return window;}

            // Declare a function to calculate the ray direction
            glm::vec3 getRayDirection(float mouseX, float mouseY) const;

            // Project 3D world coordinate to 2D screen coordinate for HUD reticles
            bool worldToScreen(const glm::vec3& worldPos, glm::vec2& outScreenPos, float& outDist) const;

            unsigned int getBlurredTexture() const { return uiPingpongColorbuffers[0]; }
    };
}
