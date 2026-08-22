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
        cameraPos = glm::vec3(0.0f, 6.0f, 12.0f);
        cameraFront = glm::vec3(0.0f, -0.4f, -1.0f);
        cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        yaw = -90.0f;
        pitch = -22.0f;
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
        initFBO();

        // Init ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        // Interface customization
        // Sleek, minimalist Onyx Grey color scheme
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.0f;
        style.FrameRounding = 0.0f;
        style.WindowBorderSize = 1.0f;

        // Configure the color with an Alpha channel (4th parameter) for transparency
        // Window background: Solid black with 60% transparency
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.0f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.80f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 0.90f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.40f, 0.40f, 0.40f, 0.30f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.70f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 0.70f);

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
            out vec3 LocalPos;

            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;

            void main() {
                LocalPos = aPos;
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
            layout (location = 0) out vec4 FragColor;       // Render Texture 1
            layout (location = 1) out vec4 BrightColor;     // Render Texture 2
            
            in vec3 FragPos;
            in vec3 Normal;
            in vec3 LocalPos; // Get origin coordinates
            uniform vec3 lightPos; 
            uniform float lightRadius; // Host star's rendered radius

            void main() {
                float ambientStrength = 0.02; 
                vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);

                vec3 norm = normalize(Normal);
                vec3 lightDir = normalize(lightPos - FragPos);
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = diff * vec3(0.9, 0.9, 0.9);

                vec3 result;
                if (length(lightPos - FragPos) < lightRadius) {
                    result = vec3(3.0, 3.0, 3.0);
                }
                else {
                    float stripe = sin(LocalPos.y * 20.0);
                    vec3 planetBaseColor = vec3(0.3) + vec3(0.08) * stripe;

                    result = (ambient + diffuse) * planetBaseColor;
                }
                
                // Output actual colors
                FragColor = vec4(result, 1.0);

                // Check: If brightness > 1.0, output it to BrightColor for the bloom effect
                float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
                if(brightness > 1.0)
                    BrightColor = vec4(result, 1.0);
                else
                    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
            }
        )glsl";

        // SCREEN SHADER (POST-PROCESSING)
        const char* screenVertexShaderSource = R"glsl(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTexCoords;
            out vec2 TexCoords;
            void main() {
                TexCoords = aTexCoords;
                gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
            }
        )glsl";

        const char* screenFragmentShaderSource = R"glsl(
            #version 330 core
            out vec4 FragColor;
            in vec2 TexCoords;
            
            uniform sampler2D screenTexture; // Scene
            uniform sampler2D bloomBlur;     // The light has been blurred

            void main() {
                vec3 hdrColor = texture(screenTexture, TexCoords).rgb;      
                vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
                
                // Additive Blending
                hdrColor += bloomColor; 

                // Tone Mapping
                // Compressing HDR lighting back into the display's LDR color space
                // The Exposure Tone Mapping algorithm creates a glare effect similar to that perceived by the human eye
                float exposure = 1.0;
                vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

                // Vignette effect
                vec2 center = TexCoords - vec2(0.5);
                float dist = length(center);
                float vignette = smoothstep(0.8, 0.2, dist);
                mapped *= vignette;

                FragColor = vec4(mapped, 1.0);
            }
        )glsl";

        const char* blurFragmentShaderSource = R"glsl(
            #version 330 core
            out vec4 FragColor;
            in vec2 TexCoords;

            uniform sampler2D image;
            uniform bool horizontal; // Determine the blurring direction
            // Gaussian weights (set from C++)
            uniform float weight[5];

            void main() {
                vec2 tex_offset = 1.0 / textureSize(image, 0);
                vec3 result = texture(image, TexCoords).rgb * weight[0]; 
                
                if(horizontal) {
                    for(int i = 1; i < 5; ++i) {
                        result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
                        result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
                    }
                } else {
                    for(int i = 1; i < 5; ++i) {
                        result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
                        result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
                    }
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

        // Compile Screen Vertex Shader
        unsigned int screenVertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(screenVertexShader, 1, &screenVertexShaderSource, NULL);
        glCompileShader(screenVertexShader);

        // Compile Screen Fragment Shader
        unsigned int screenFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(screenFragmentShader, 1, &screenFragmentShaderSource, NULL);
        glCompileShader(screenFragmentShader);

        // Link screen shader program
        screenShaderProgram = glCreateProgram();
        glAttachShader(screenShaderProgram, screenVertexShader);
        glAttachShader(screenShaderProgram, screenFragmentShader);
        glLinkProgram(screenShaderProgram);

        // Delete temporary screen shader files
        glDeleteShader(screenVertexShader);
        glDeleteShader(screenFragmentShader);

        // Compile Blur Vertex Shader (reuses the screen-space vertex shader)
        unsigned int blurVertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(blurVertexShader, 1, &screenVertexShaderSource, NULL);
        glCompileShader(blurVertexShader);

        // Compile Blur Fragment Shader
        unsigned int blurFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(blurFragmentShader, 1, &blurFragmentShaderSource, NULL);
        glCompileShader(blurFragmentShader);

        // Link blur shader program
        blurShaderProgram = glCreateProgram();
        glAttachShader(blurShaderProgram, blurVertexShader);
        glAttachShader(blurShaderProgram, blurFragmentShader);
        glLinkProgram(blurShaderProgram);

        // Delete temporary blur shader files
        glDeleteShader(blurVertexShader);
        glDeleteShader(blurFragmentShader);
    }

    void Renderer::initSphere(int sectorCount, int stackCount) {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        float radius = 1.0f;
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
        glDeleteProgram(screenShaderProgram);
        glDeleteProgram(blurShaderProgram);

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

    void Renderer::draw(size_t count, const std::vector<Vector3>& positions, const std::vector<double>& radii, const std::vector<glm::quat>& orientations) const {
        // Render the 3D universe to a Frame Buffer Object (FBO)
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
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
        int lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
        int lightRadiusLoc = glGetUniformLocation(shaderProgram, "lightRadius");
        if (count > 0) {
            glUniform3f(lightPosLoc, (float)positions[0].x, (float)positions[0].y, (float)positions[0].z);
            // Host star's rendered radius
            glUniform1f(lightRadiusLoc, (float)radii[0]);
        }

        for (size_t i = 0; i < count; ++i) {
            glm::mat4 model = glm::mat4(1.0f); 
            glm::vec3 pos((float)positions[i].x, (float)positions[i].y, (float)positions[i].z);
            float radius = (float)radii[i];

            // Displacement Matrix
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), pos);
            // Rotation Matrix (Convert from Quat to Mat4)
            glm::mat4 rotation = glm::mat4_cast(orientations[i]);
            // Elasticity Matrix
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(radius));

            // Assemble in the correct order: T * R * S
            model = translation * rotation * scale;

            // Send this object's own model matrix to the GPU
            glUniformMatrix4fv(modeLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Draw command: Drawing 36 vertices from VBO (Forming a cube)
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        }

        // Appy Gaussian Blur
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;

        glActiveTexture(GL_TEXTURE0);
        glUseProgram(blurShaderProgram);
        glUniform1i(glGetUniformLocation(blurShaderProgram, "image"), 0);
        float weights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };
        glUniform1fv(glGetUniformLocation(blurShaderProgram, "weight"), 5, weights);
        for (unsigned int i = 0; i < amount; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            glUniform1i(glGetUniformLocation(blurShaderProgram, "horizontal"), horizontal);

            // Get image from main FBO
            glBindTexture(GL_TEXTURE_2D, first_iteration ? textureColorbuffer : pingpongColorbuffers[!horizontal]);

            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            horizontal = !horizontal;
            if (first_iteration) {
                first_iteration = false;
            }
        }

        // Apply Gaussian Blur for Bloom
        bool horizontal_bloom = true;
        bool first_iteration_bloom = true;
        unsigned int amount_bloom = 15;

        glUseProgram(blurShaderProgram);
        for (unsigned int i = 0; i < amount_bloom; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO_Bloom[horizontal_bloom]);
            glUniform1i(glGetUniformLocation(blurShaderProgram, "horizontal"), horizontal_bloom);

            glBindTexture(GL_TEXTURE_2D, first_iteration_bloom ? textureBloombuffer : pingpongColorbuffers_Bloom[!horizontal_bloom]);

            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            horizontal_bloom = !horizontal_bloom;
            if (first_iteration_bloom) {
                first_iteration_bloom = false;
            }
        }

        // Paste the FBO image onto the screen and apply a filter
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(screenShaderProgram);

        // Assign the scenery texture to Slot 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        glUniform1i(glGetUniformLocation(screenShaderProgram, "screenTexture"), 0);

        // Attach the blurred bloom texture to Slot 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers_Bloom[!horizontal_bloom]);
        glUniform1i(glGetUniformLocation(screenShaderProgram, "bloomBlur"), 1);
        
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Unbind VAO after drawing is complete
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
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

    void Renderer::initFBO() {
        // Create FBO
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        // Create a texture to store the colors of the universe
        glGenTextures(1, &textureColorbuffer);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Assign Texture to FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

        // Bloom effect
        glGenTextures(1, &textureBloombuffer);
        glBindTexture(GL_TEXTURE_2D, textureBloombuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, textureBloombuffer, 0);

        unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);

        // Create RBP to processing Depth
        glGenRenderbuffers(1, &RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

        // Return default FBO of display
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Init Screen Quad
        float quadVertices[] = {
            // Coordinates (X, Y)   // Texture Coordinate (U, V)
            -1.0f,  1.0f,           0.0f, 1.0f,
            -1.0f, -1.0f,           0.0f, 0.0f,
             1.0f, -1.0f,           1.0f, 0.0f,
            -1.0f,  1.0f,           0.0f, 1.0f,
             1.0f, -1.0f,           1.0f, 0.0f,
             1.0f,  1.0f,           1.0f, 1.0f
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        // Init Ping Pong FBOs for Gaussian Blur
        glGenFramebuffers(2, pingpongFBO);
        glGenTextures(2, pingpongColorbuffers);
        for (unsigned int i = 0; i < 2; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
            glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        }

        // Init Ping-Pong FBOs for bloom
        glGenFramebuffers(2, pingpongFBO_Bloom);
        glGenTextures(2, pingpongColorbuffers_Bloom);
        for (unsigned int i = 0; i < 2; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO_Bloom[i]);
            glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers_Bloom[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers_Bloom[i], 0);
        }
    }
}