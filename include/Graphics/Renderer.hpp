#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Graphics {
    class Renderer {
        private:
            GLFWwindow* window;
            int width;
            int height;
        
        public:
            Renderer(int w, int h, const char* title);

            // Destructor
            ~Renderer();

            bool shouldClose() const;
            void clear() const;
            void swapBuffers() const;
            void pollEvents() const;
    };
}