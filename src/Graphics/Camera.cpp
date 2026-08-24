#include <Graphics/Camera.hpp>
#include <cmath>

namespace Graphics {
    Camera::Camera(glm::vec3 position)
        : Position(position),
          Front(glm::vec3(0.0f, 0.0f, -1.0f)),
          Up(glm::vec3(0.0f, 1.0f, 0.0f)),
          Right(glm::vec3(1.0f, 0.0f, 0.0f)),
          WorldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
          Yaw(-90.0f),
          Pitch(-22.0f),
          MovementSpeed(50.0f),
          MouseSensitivity(0.1f),
          lockedTargetIndex(-1),
          orbitDistance(20.0f),
          orbitTheta(0.0f),
          orbitPhi(0.0f),
          targetRadius(0.05f),
          scrollAccum(0.0f),
          firstMouse(true),
          lastX(0.0f),
          lastY(0.0f),
          uiMode(false) {
        updateCameraVectors();
    }

    glm::mat4 Camera::getViewMatrix() const {
        return glm::lookAt(Position, Position + Front, Up);
    }

    glm::mat4 Camera::getViewMatrixNoTranslation() const {
        return glm::mat4(glm::mat3(getViewMatrix()));
    }

    void Camera::processInput(GLFWwindow* window, float deltaTime) {
        // Cursor is visible when in persistent UI mode OR while temporarily
        // unlocking with Left Alt (used for in-space raycast selection).
        bool cursorVisible = uiMode || (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS);
        if (cursorVisible) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstMouse = true; // Prevent camera jitter when re-hiding the mouse cursor
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        // Mode 1: Lock Target
        // Camera follow planetary orbit
        if (lockedTargetIndex != -1) {
            // W,S to narrow or widen the viewing distance.
            // Zoom is exponential (logarithmic) so we can efficiently travel from
            // the whole system (~100 AU) down to a single planet (~1e-4 AU).
            float zoomRate = 0.6f * MovementSpeed * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                orbitDistance *= std::exp(-zoomRate);
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                orbitDistance *= std::exp(zoomRate);
            }

            // Mouse wheel zoom (exponential, same feel as W/S)
            if (scrollAccum != 0.0f) {
                orbitDistance *= std::exp(-scrollAccum * 0.25f);
                scrollAccum = 0.0f;
            }

            // Clamp so we can never dive inside the planet (radius + a small margin)
            // but still reach deep orbit down to a tiny fraction of the planet's size.
            float minDist = std::max(targetRadius * 2.0f, 1.0e-5f);
            float maxDist = 120.0f; // comfortably outside the whole solar system
            if (orbitDistance < minDist) {
                orbitDistance = minDist;
            }
            if (orbitDistance > maxDist) {
                orbitDistance = maxDist;
            }

            // If the cursor is hidden (space mode), allow orbit rotation
            if (!cursorVisible) {
                double xpos, ypos;

                glfwGetCursorPos(window, &xpos, &ypos);
                if (firstMouse) {
                    lastX = (float)xpos;
                    lastY = (float)ypos;
                    firstMouse = false;
                }

                float xoffset = (float)xpos - lastX;
                float yoffset = (float)ypos - lastY;
                lastX = (float)xpos;
                lastY = (float)ypos;

                float sensitivity = 0.005f;
                orbitTheta += xoffset * sensitivity;
                orbitPhi += yoffset * sensitivity;

                // Lock the tilt angle to prevent the camera from flipping over
                if (orbitPhi > 1.5f) orbitPhi = 1.5f;
                if (orbitPhi < -1.5f) orbitPhi = -1.5f;
            }
        }

        // Mode 2
        // Free-Fly
        else {
            // Scale movement speed to the current "view scale" so you can cruise
            // across AU-scale distances yet still manoeuvre precisely around a
            // planet. MovementSpeed is a user-set baseline (units/s).
            float viewScale = clampViewScale();
            float moveSpeed = MovementSpeed * viewScale * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                Position += moveSpeed * Front;
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                Position -= moveSpeed * Front;
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                Position -= Right * moveSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                Position += Right * moveSpeed;
            }

            if (!cursorVisible) {
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                if (firstMouse) {
                    lastX = (float)xpos;
                    lastY = (float)ypos;
                    firstMouse = false;
                }

                float xoffset = (float)xpos - lastX;
                float yoffset = lastY - (float)ypos;
                lastX = (float)xpos;
                lastY = (float)ypos;

                Yaw += xoffset * MouseSensitivity;
                Pitch += yoffset * MouseSensitivity;
                if (Pitch > 89.0f) Pitch = 89.0f;
                if (Pitch < -89.0f) Pitch = -89.0f;

                updateCameraVectors();
            }
        }
    }

    void Camera::lockTarget(int entityIndex, float distance, float planetRadius) {
        lockedTargetIndex = entityIndex;
        targetRadius = planetRadius;
        orbitDistance = distance;
        orbitTheta = 0.0f;
        orbitPhi = 0.0f;
        firstMouse = true;
    }

    void Camera::unlockTarget() {
        lockedTargetIndex = -1;
    }

    bool Camera::isTargetLocked() const {
        return lockedTargetIndex != -1;
    }

    void Camera::updateTracking(const std::vector<Vector3>& positions) {
        if (lockedTargetIndex != -1 && static_cast<size_t>(lockedTargetIndex) < positions.size()) {
            Vector3 target = positions[lockedTargetIndex];
            glm::vec3 targetPos((float)target.x, (float)target.y, (float)target.z);

            // Use trigonometry to convert spherical coordinates (orbitTheta, orbitPhi, orbitDistance) into Cartesian coordinates (X, Y, Z)
            float camX = orbitDistance * cos(orbitPhi) * cos(orbitTheta);
            float camY = orbitDistance * sin(orbitPhi);
            float camZ = orbitDistance * cos(orbitPhi) * sin(orbitTheta);

            // Lock the camera onto the target
            Position = targetPos + glm::vec3(camX, camY, camZ);

            // Force the camera to keep its view (front) aimed directly at the center of the target
            Front = glm::normalize(targetPos - Position);
        }
    }

    void Camera::updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);

        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }

    float Camera::clampViewScale() const {
        // Distance from the camera to the thing it's orbiting (or, when free-
        // flying, a nominal scale). Larger scale -> faster traversal so the
        // camera can cross AU distances in seconds; small scale -> slow,
        // precise movement around a small planet.
        float scale = (lockedTargetIndex != -1) ? orbitDistance : 1.0f;
        if (scale < 5.0e-4f) scale = 5.0e-4f;
        if (scale > 40.0f) scale = 40.0f;
        return scale;
    }
}
