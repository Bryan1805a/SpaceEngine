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

            // Cursor status
            bool firstMouse;
            float lastX;
            float lastY;

            // Default constructor
            Camera(glm::vec3 position = glm::vec3(0.0f, 6.0f, 12.0f));

            // Get view matrix
            glm::mat4 getViewMatrix() const;

            // View matrix but remove translation (For skybox)
            glm::mat4 getViewMatrixNoTranslation() const;

            // Free-fly and rotate
            void processInput(GLFWwindow* window, float deltaTime);

            // API Tracking
            void lockTarget(int entityIndex, float distance);
            void unlockTarget();
            bool isTargetLocked() const;
            void updateTracking(const std::vector<Vector3>& positions);
        
        private:
            // Update vector Front, Right, Up based on Yaw/Pitch
            void updateCameraVectors();
    };
}
