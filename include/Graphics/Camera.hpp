#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <Math/Vector3.hpp>

namespace Graphics {
    class Camera {
        public:
            glm::vec3 Position;
            glm::vec3 Front;
            glm::vec3 Up;
            glm::vec3 Right;
            glm::vec3 WorldUp;

            // Euler angle
            float Yaw;
            float Pitch;

            // Camera setting
            float MovementSpeed;
            float MouseSensitivity;

            // Camera tracking status (Lock target)
            int lockedTargetIndex;
            float orbitDistance;
            float orbitTheta;
            float orbitPhi;

            // Radius of the currently locked target (in world units).
            // Used to clamp the zoom so the camera never orbits inside a planet.
            float targetRadius;

            // Cursor status
            bool firstMouse;
            float lastX;
            float lastY;

            // When true, the UI cursor stays visible and mouse-look is disabled
            bool uiMode;

            // Accumulated mouse-wheel delta consumed during the next input pass
            float scrollAccum;

            // Default constructor
            Camera(glm::vec3 position = glm::vec3(0.0f, 6.0f, 12.0f));

            // Get view matrix
            glm::mat4 getViewMatrix() const;

            // View matrix but remove translation (For skybox)
            glm::mat4 getViewMatrixNoTranslation() const;

            // Free-fly and rotate
            void processInput(GLFWwindow* window, float deltaTime);

            // API Tracking
            void lockTarget(int entityIndex, float distance, float planetRadius = 0.05f);
            void unlockTarget();
            bool isTargetLocked() const;
            void updateTracking(const std::vector<Vector3>& positions);

            // Toggle the persistent UI cursor mode (Tab key)
            void setUIMode(bool enabled) { uiMode = enabled; }

            // Mouse-wheel zoom for the locked-orbit camera
            void addScroll(float yoffset) { scrollAccum += yoffset; }
        
        private:
            // Update vector Front, Right, Up based on Yaw/Pitch
            void updateCameraVectors();

            // A smoothly-varying scale that adapts the free-fly speed to the
            // size of the region being viewed (bounded to avoid runaway speeds).
            float clampViewScale() const;
    };
}
