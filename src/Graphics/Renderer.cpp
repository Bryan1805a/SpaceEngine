#include <iostream>
#include <cmath>
#include <vector>
#include <Graphics/Renderer.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

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

        // Init default camera status
        cameraPos = glm::vec3(0.0f, 150.0f, 300.0f);
        cameraFront = glm::vec3(0.0f, -0.5f, -1.0f);
        cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        yaw = -90.0f;
        pitch = -26.5f;
        lastX = width / 2.0f;
        lastY = height / 2.0f;
        firstMouse = true;
        // Lock the mouse cursor to the center of the screen and hide it
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Set the background color for the space environment
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);

        // Depth Testing
        glEnable(GL_DEPTH_TEST);

        // Preparing GPU data
        initShaders();
        initSphere(36, 18);

        // Init ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        // Interface customization
        // Sleek, minimalist Onyx Grey color scheme
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.0f;
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.95f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);

        // Connect backend
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
    }

    void Renderer::initShaders() {
        // Vertex shader code
        const char* vertexShaderSource = R"glsl(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec3 aNormal;

            out vec3 FragPos; // Passing world coordinates to the fragment shader
            out vec3 Normal; // Passing normal to the fragment shader

            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;

            void main() {
                FragPos = vec3(model * vec4(aPos, 1.0));

                // Normal matrix
                // Avoid distortion when scaling
                Normal = mat3(transpose(inverse(model))) * aNormal;

                gl_Position = projection * view * vec4(FragPos, 1.0);
            }
        )glsl";

        // Fragment Shader source code
        const char* fragmentShaderSource = R"glsl(
            #version 330 core
            out vec4 FragColor;
            
            in vec3 FragPos;
            in vec3 Normal;

            uniform vec3 objectColor;
            uniform vec3 lightPos; // Light source position (Host star)
            uniform float lightRadius; // Host star's rendered radius

            void main() {
                // Ambient: Very dark to create a tranquil atmosphere
                float ambientStrength = 0.05; 
                vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);

                // Diffuse scattering
                // Illuminates only the side facing the light source
                vec3 norm = normalize(Normal);
                vec3 lightDir = normalize(lightPos - FragPos);
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = diff * vec3(1.0, 1.0, 0.9); // The light has a slight yellowish tint

                // Disable shadow casting for the main star (it emits its own light).
                vec3 result;
                if (length(lightPos - FragPos) < lightRadius) {
                    result = objectColor; // The main star does not become dim
                } else {
                    result = (ambient + diffuse) * objectColor;
                }
                
                FragColor = vec4(result, 1.0);
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

    void Renderer::initSphere(int sectorCount, int stackCount) {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        float radius = 0.5f;
        float sectorStep = 2 * 3.14159265359f / sectorCount;
        float stackStep = 3.14159265359f / stackCount;
        float sectorAngle, stackAngle;

        // Generating Vertex Coordinates and Normal Vectors
        for (int i = 0; i <= stackCount; ++i) {
            stackAngle = 3.14159265359f / 2 - i * stackStep; // The angle from π/2 to -π/2
            float xy = radius * std::cos(stackAngle);
            float z = radius * std::sin(stackAngle);

            for (int j = 0; j <= sectorCount; ++j) {
                sectorAngle = j * sectorStep; // The angle from 0 to 2*π

                // Vertex coordinates (Position)
                float x = xy * std::cos(sectorAngle);
                float y = xy * std::sin(sectorAngle);
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);

                // Normal vector - Used to calculate incident light
                // For a sphere centered at (0, 0, 0)
                // The normal vector is simply the normalized vertex coordinates (divided by the radius)
                vertices.push_back(x / radius);
                vertices.push_back(y / radius);
                vertices.push_back(z / radius);
            }
        }

        // Generate indices to connect vertices into triangles
        for (int i = 0; i < stackCount; ++i) {
            int k1 = i * (sectorCount + 1); // Head of current parallel
            int k2 = k1 + sectorCount + 1; // Head of next parallel

            for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
                if (i != 0) {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                }
                if (i != (stackCount - 1)) {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                }
            }
        }
        indexCount = indices.size();

        // Load data to VRAM (Including EBO)
        unsigned int EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO); // Element Buffer Object is used to store an array of indices

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // Declare how to read VBO
        // Now each vertex has 6 real numbers: 3 Coordinates + 3 Normals
        int stride = 6 * sizeof(float);
        // Attribute 0: Coordinates (aPos)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        // Attribute 1: aNormal - Shift by 3 floats
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    Renderer::~Renderer() {
        // Clean ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

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

    void Renderer::draw(size_t count, const std::vector<Vector3>& positions, const std::vector<double>& masses) const {
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
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Drawing objects
        // Bind the VAO again to let the GPU know we are about to use the cube's vertex data
        glBindVertexArray(VAO);

        int modeLoc = glGetUniformLocation(shaderProgram, "model");
        int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
        int lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
        int lightRadiusLoc = glGetUniformLocation(shaderProgram, "lightRadius");
        if (count > 0) {
            glUniform3f(lightPosLoc, (float)positions[0].x, (float)positions[0].y, (float)positions[0].z);
            // World-space radius = base sphere radius (0.5) * model scale (cbrt(mass))
            glUniform1f(lightRadiusLoc, 0.5f * (float)std::cbrt(masses[0]));
        }

        for (size_t i = 0; i < count; ++i) {
            // Model matrix
            // Start with identity matrix standing at Origin
            glm::mat4 model = glm::mat4(1.0f);

            // Move the cube based on the position calculated from physics
            glm::vec3 pos((float)positions[i].x, (float)positions[i].y, (float)positions[i].z);
            model = glm::translate(model, pos);


            // Categorise volumes to scaling and colour-coding to improve visibility
            // Size is proportional to mass (cube root)
            float radius = (float)std::cbrt(masses[i]);
            model = glm::scale(model, glm::vec3(radius, radius, radius));
            
            // Color temperature: The larger the mass, the brighter the color 
            // Red -> Blue -> White/Yellow
            if (masses[i] >= 1000.0) {
                glUniform3f(colorLoc, 1.0f, 0.8f, 0.2f); // Supermassive star
            }
            else if (masses[i] >= 50.0) {
                glUniform3f(colorLoc, 0.2f, 0.6f, 1.0f); // Gas giant
            }
            else {
                glUniform3f(colorLoc, 0.7f, 0.3f, 0.2f); // Rocky asteroid
            }

            // Send this object's own model matrix to the GPU
            glUniformMatrix4fv(modeLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Draw command: Drawing 36 vertices from VBO (Forming a cube)
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        }

        // Unbind VAO after drawing is complete
        glBindVertexArray(0);
    }

    void Renderer::pollEvents() const {
        glfwPollEvents();
    }

    void Renderer::processInput(float deltaTime) {
        // If escape button pressed -> close the window
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // Handle W, A, S, D key movement
        // Flight speed: 150 units/second
        float cameraSpeed = 150.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            cameraPos += cameraSpeed * cameraFront;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cameraPos -= cameraSpeed * cameraFront;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        }


        // Unlock mouse cursor while holding Left Alt, re-lock on release
        if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstMouse = true; // Prevent camera jitter when hiding the mouse cursor
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            // Handling Viewpoint Rotation with the Mouse
            // Using Trigonometry
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            if (firstMouse) {
                lastX = (float)xpos;
                lastY = (float)ypos;
                firstMouse = false;
            }

            float xoffset = (float)xpos - lastX;
            float yoffset = lastY - (float)ypos; // The mouse's Y-axis is inverted relative to 3D coordinates
            lastX = (float)xpos;
            lastY = (float)ypos;

            float sensitivity = 0.1f;
            xoffset *= sensitivity;
            yoffset *= sensitivity;

            yaw += xoffset;
            pitch += yoffset;

            // Lock the tilt angle to prevent the camera from flipping over
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;

            // Recalculate the view direction vector using sine and cosine
            glm::vec3 front;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            cameraFront = glm::normalize(front);
        }
    }

    void Renderer::beginUI() const {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void Renderer::endUI() const {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    glm::vec3 Renderer::getRayDirection(float mouseX, float mouseY) const {
        // Convert pixel coordinates to normalized NDC coordinates (-1.0 to 1.0)
        float x = (2.0f * mouseX) / width - 1.0f;
        float y = 1.0f - (2.0f * mouseY) / height; // The screen's Y-axis is inverted relative to the 3D view
        glm::vec4 ray_clip(x, y, -1.0f, 1.0f); // The ray starts from the near plane

        // View Space with Inverse Matrix
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
        glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
        ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f); // Set w=0 to turn it into a direction vector

        // View Space to World Space
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::vec3 ray_wor = glm::vec3(glm::inverse(view) * ray_eye);

        // Returns the normalized direction vector
        return glm::normalize(ray_wor);
    }
}