#include <Graphics/Renderer.hpp>
#include <iostream>

namespace Graphics {
    Renderer::Renderer(int w, int h, const char* title)
        : width(w), height(h), window(nullptr) {
            if (!glfwInit()) {
                std::cerr << "ERROR: Cannot init GLFW" << std::endl;
                return;
            }

            // Configure OpenGL
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

            window = glfwCreateWindow(width, height, title, nullptr, nullptr);
            if (!window) {
                std::cerr << "ERROR: Cannot init GLFW window" << std::endl;
                glfwTerminate();
                return;
            }

            // Assign context into this processing thread
            glfwMakeContextCurrent(window);

            // Init GLAD
            if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
                std::cerr << "ERROR: Cannot init GLAD" << std::endl;
                return;
            }

            // Set the background color for the space environment
            glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        }

        Renderer::~Renderer() {
            if (window) {
                glfwDestroyWindow(window);
            }
            glfwTerminate();
        }

        bool Renderer::shouldClose() const {
            return glfwWindowShouldClose(window);
        }

        void Renderer::clear() const {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        void Renderer::swapBuffers() const {
            glfwSwapBuffers(window);
        }

        void Renderer::pollEvents() const {
            glfwPollEvents();
        }
}