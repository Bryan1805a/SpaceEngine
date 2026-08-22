#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
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
        cameraBaseSpeed = 50.0f;
        lockedTargetIndex = -1;
        orbitDistance = 20.0f;
        orbitTheta = 0.0f;
        orbitPhi = 0.0f;
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
        // Load, compile and link each shader program from its source files
        shaderProgram = loadShaderFromFile("assets/shaders/planet.vert", "assets/shaders/planet.frag");
        screenShaderProgram = loadShaderFromFile("assets/shaders/screen.vert", "assets/shaders/screen.frag");
        blurShaderProgram = loadShaderFromFile("assets/shaders/screen.vert", "assets/shaders/blur.frag");
        skyboxShaderProgram = loadShaderFromFile("assets/shaders/skybox.vert", "assets/shaders/skybox.frag");
        shadowShaderProgram = loadShaderFromFile("assets/shaders/shadow.vert", "assets/shaders/shadow.frag", "assets/shaders/shadow.geom");
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

    void Renderer::draw(size_t count, const std::vector<Vector3>& positions, const std::vector<double>& radii, const std::vector<glm::quat>& orientations, const std::vector<Simulation::BodyType>& types, const std::vector<double>& temperatures) const {
        // Calculate 6 matrices from the Sun's perspective (90-degree field of view)
        float near_plane = 1.0f;
        float far_plane = 250.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);

        glm::vec3 lightPos(0.0f);
        if (count > 0) {
            lightPos = glm::vec3((float)positions[0].x, (float)positions[0].y, (float)positions[0].z);
        }

        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0)));

        // SHADOW MAP
        glViewport(0, 0, SHADOW_RES, SHADOW_RES);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glUseProgram(shadowShaderProgram);
        glBindVertexArray(VAO);

        for (unsigned int i = 0; i < 6; ++i) {
            glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, ("shadowMatrices[" + std::to_string(i) + "]").c_str()), 1, GL_FALSE, glm::value_ptr(shadowTransforms[i]));
        }

        glUniform1f(glGetUniformLocation(shadowShaderProgram, "far_plane"), far_plane);
        glUniform3f(glGetUniformLocation(shadowShaderProgram, "lightPos"), lightPos.x, lightPos.y, lightPos.z);

        // Drawing loop
        for (size_t i = 1; i < count; ++i) { // Exclude the Sun (i=0) because the Sun does not cast a shadow on itself
            glm::mat4 model = glm::mat4(1.0f);
            glm::vec3 pos((float)positions[i].x, (float)positions[i].y, (float)positions[i].z);
            float radius = (float)radii[i];
            model = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(orientations[i]) * glm::scale(glm::mat4(1.0f), glm::vec3(radius));

            glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        
        // Render the 3D universe to a Frame Buffer Object (FBO)
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Activate Shader Program
        glUseProgram(shaderProgram);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
        glUniform1i(glGetUniformLocation(shaderProgram, "depthMap"), 2);
        glUniform1f(glGetUniformLocation(shaderProgram, "far_plane"), far_plane);

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

        // Send camera position to the atmospheric (Fresnel) shader
        int viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
        glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);

        // // Send light source position (Fixed at the Star - position 0)
        int lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
        if (count > 0) {
            glUniform3f(lightPosLoc, (float)positions[0].x, (float)positions[0].y, (float)positions[0].z);
        }

        int typeLoc = glGetUniformLocation(shaderProgram, "bodyType");
        int tempLoc = glGetUniformLocation(shaderProgram, "temperature");

        // Draw SKYBOX
        glDepthFunc(GL_LEQUAL);
        glUseProgram(skyboxShaderProgram);

        // Remove Translation part of Camera
        glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));

        // Inverse matrix
        glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "invProjection"), 1, GL_FALSE, glm::value_ptr(glm::inverse(projection)));
        glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "invView"), 1, GL_FALSE, glm::value_ptr(glm::inverse(viewNoTranslation)));

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDepthFunc(GL_LESS);

        // Switch back to the main planet shader before drawing the bodies
        glUseProgram(shaderProgram);

        // Drawing objects
        // Bind the VAO again to let the GPU know we are about to use the cube's vertex data
        glBindVertexArray(VAO);

        int modeLoc = glGetUniformLocation(shaderProgram, "model");

        for (size_t i = 0; i < count; ++i) {
            glm::mat4 model = glm::mat4(1.0f); 
            glm::vec3 pos((float)positions[i].x, (float)positions[i].y, (float)positions[i].z);

            // Visual scale (explicit radius in AU)
            float radius = (float)radii[i];

            // Displacement Matrix
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), pos);
            // Rotation Matrix (Convert from Quat to Mat4)
            glm::mat4 rotation = glm::mat4_cast(orientations[i]);
            // Elasticity Matrix
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(radius));

            // Assemble in the correct order: T * R * S
            model = translation * rotation * scale;

            // Send the object types and temperature to the GPU
            glUniform1i(typeLoc, static_cast<int>(types[i]));
            glUniform1f(tempLoc, (float)temperatures[i]);

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

        // Unlock mouse cursor while holding Left Alt, re-lock on release
        if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstMouse = true; // Prevent camera jitter when hiding the mouse cursor
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        
        // Mode 1: Lock Target
        // Camera follow plantary orbit
        if (lockedTargetIndex != -1) {
            // W,S to narrow or widen the viewing distance
            float zoomSpeed = cameraBaseSpeed * 0.5f * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                orbitDistance -= zoomSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                orbitDistance += zoomSpeed;
            }
            if (orbitDistance < 2.0f) {
                orbitDistance = 2.0f;
            }
            if (orbitDistance > 500.0f) {
                orbitDistance = 500.0f;
            }

            // If left Alt is not pressed
            if (!(glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)) {
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
            float moveSpeed = cameraBaseSpeed * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                cameraPos += moveSpeed * cameraFront;
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                cameraPos -= moveSpeed * cameraFront;
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * moveSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * moveSpeed;
            }

            if (!(glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)) {
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

                float sensitivity = 0.1f;
                yaw += xoffset * sensitivity;
                pitch += yoffset * sensitivity;
                if (pitch > 89.0f) pitch = 89.0f;
                if (pitch < -89.0f) pitch = -89.0f;

                glm::vec3 front;
                front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                front.y = sin(glm::radians(pitch));
                front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                cameraFront = glm::normalize(front);
            }
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
        // Create Framebuffer for Shadow
        glGenFramebuffers(1, &depthMapFBO);
        glGenTextures(1, &depthCubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);

        // Create 6 empty Texture faces
        for (unsigned int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, 
                         SHADOW_RES, SHADOW_RES, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }

        // Prevent cracking or crazing of the finish along the edges
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        // Attach to the FBO and tell OpenGL not to output color
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

    void Renderer::lockTarget(int entityIndex, float distance) {
        lockedTargetIndex = entityIndex;
        orbitDistance = distance;
        orbitTheta = 0.0f;
        orbitPhi = 0.0f;

        // When locking onto a target, hide the cursor to use the mouse for rotating the camera orbit
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true;
    }

    void Renderer::unlockTarget() {
        lockedTargetIndex = -1;
    }

    void Renderer::updateCameraTracking(const std::vector<Vector3>& positions) {
        if (lockedTargetIndex != -1 && static_cast<size_t>(lockedTargetIndex) < positions.size()) {
            Vector3 target = positions[lockedTargetIndex];
            glm::vec3 targetPos((float)target.x, (float)target.y, (float)target.z);

            // Use trigonometry to convert spherical coordinates (orbitTheta, orbitPhi, orbitDistance) into Cartesian coordinates (X, Y, Z)
            float camX = orbitDistance * cos(orbitPhi) * cos(orbitTheta);
            float camY = orbitDistance * sin(orbitPhi);
            float camZ = orbitDistance * cos(orbitPhi) * sin(orbitTheta);

            // Lock the camera onto the target
            cameraPos = targetPos + glm::vec3(camX, camY, camZ);

            // Force the camera to keep its view (front) aimed directly at the center of the target
            cameraFront = glm::normalize(targetPos - cameraPos);
        }
    }

    unsigned int Renderer::loadShaderFromFile(const char* vertexPath, const char* fragmentPath, const char* geometryPath) {
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            // Read file content
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // Close file
            vShaderFile.close();
            fShaderFile.close();
            // Convert stream to string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        } catch (std::ifstream::failure& e) {
            std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << vertexPath << " or " << fragmentPath << std::endl;
        }

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        unsigned int vertex, fragment;
        int success;
        char infoLog[512];

        // Compile Vertex Shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        // Compile Fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        // Compile Geometry Shader
        unsigned int geometry = 0;
        if (geometryPath != nullptr) {
            std::string geometryCode;
            std::ifstream gShaderFile;
            gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            try {
                gShaderFile.open(geometryPath);
                std::stringstream gShaderStream;
                gShaderStream << gShaderFile.rdbuf();
                gShaderFile.close();
                geometryCode = gShaderStream.str();
            } catch (std::ifstream::failure& e) {
                std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << geometryPath << std::endl;
            }
            const char* gShaderCode = geometryCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, NULL);
            glCompileShader(geometry);
        }

        // Link Shader Program
        unsigned int ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if (geometryPath != nullptr) {
            glAttachShader(ID, geometry);
        }
        glLinkProgram(ID);
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }

        // Clean trash
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (geometryPath != nullptr) glDeleteShader(geometry);

        return ID;
    }
}