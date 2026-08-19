#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
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
            void initCube();
        
        public:
            Renderer(int w, int h, const char* title);

            // Destructor
            ~Renderer();

            bool shouldClose() const;
            void clear() const;
            void swapBuffers() const;
            void pollEvents() const;

            void draw(size_t count, const std::vector<Vector3>& positions, const std::vector<double>& masses) const;
    };
}