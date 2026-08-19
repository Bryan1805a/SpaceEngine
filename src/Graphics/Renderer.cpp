#include <Graphics/Renderer.hpp>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Graphics {
    Renderer::Renderer(int w, int h, const char* title) : width(w), height(h), window(nullptr) {
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

        // Depth Testing
        glEnable(GL_DEPTH_TEST);

        // Preparing GPU data
        initShaders();
        initCube();
    }

    void Renderer::initShaders() {
        // Vertex shader code
        const char* vertexShaderSource = R"glsl(
            #version 330 core
            layout (location = 0) in vec3 aPos;

            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;

            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
            }
        )glsl";

        // Fragment Shader source code
        const char* fragmentShaderSource = R"glsl(
            #version 330 core
            out vec4 FragColor;

            uniform vec3 objectColor;

            void main() {
                FragColor = vec4(objectColor, 1.0);
            }
        )glsl";

        // Compile Vertex Shader
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        // Error check will be add later

        // Compile Fragment Shader
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        // Link
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        // Delete temporary shader files
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void Renderer::initCube() {
        float vertices[] = {
            -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f
        };

        // Allocate 1 VAO and VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // Bind VAO
        glBindVertexArray(VAO);

        // Bind VBO and push vertices array from RAM to VRAM
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Unlock VAO and VBO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    Renderer::~Renderer() {
        // Clean CPU resources before closing window
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);

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

    void Renderer::draw(const std::vector<Physics::Body>& bodies) const {
        // Activate Shader Program
        glUseProgram(shaderProgram);

        // Setup CAMERA and SPACE

        // Projection Matrix
        // Creates a perspective effect (45 FOV, view range from 0.1 to 1000.0)
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
        int projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // View Matrix
        // Position the camera high up (Y=150) and set it back (Z=300)
        // Point the camera direcly down at the origin (0, 0, 0)
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 150.0f, 300.0f),
                                     glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));
        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Drawing objects
        // Bind the VAO again to let the GPU know we are about to use the cube's vertex data
        glBindVertexArray(VAO);

        int modeLoc = glGetUniformLocation(shaderProgram, "model");
        int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");

        for (size_t i = 0; i < bodies.size(); ++i) {
            // Model matrix
            // Start with identity matrix standing at Origin
            glm::mat4 model = glm::mat4(1.0f);

            // Move the cube based on the position calculated from physics
            glm::vec3 pos((float)bodies[i].position.x, (float)bodies[i].position.y, (float)bodies[i].position.z);
            model = glm::translate(model, pos);

            // Categorise volumes to scaling and colour-coding to improve visibility
            if (i == 0) { // A large star
                model = glm::scale(model, glm::vec3(10.0f, 10.0f, 10.0f));
                glUniform3f(colorLoc, 1.0f, 0.8f, 0.2f); // Yellow
            }
            else if (i == 1) {
                model = glm::scale(model, glm::vec3(3.0f, 3.0f, 3.0f));
                glUniform3f(colorLoc, 0.2f, 0.6f, 1.0f); // Blue
            }
            else {
                model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
                glUniform3f(colorLoc, 1.0f, 0.3f, 0.2f); // Red
            }

            // Send this object's own model matrix to the GPU
            glUniformMatrix4fv(modeLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Draw command: Drawing 36 vertices from VBO (Forming a cube)
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Unbind VAO after drawing is complete
        glBindVertexArray(0);
    }
}