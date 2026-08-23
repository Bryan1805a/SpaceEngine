#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <vector>
#include <Graphics/Renderer.hpp>
#include <Graphics/Shader.hpp>
#include <Graphics/Mesh.hpp>
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

        // Boot straight into fullscreen on the primary monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // Remember the windowed geometry (centered) so F11 can restore it
        windowedWidth = w;
        windowedHeight = h;
        windowedX = (mode->width - w) / 2;
        windowedY = (mode->height - h) / 2;

        window = glfwCreateWindow(mode->width, mode->height, title, monitor, nullptr);
        if (!window) {
        std::cerr << "ERROR: Cannot init GLFW window" << std::endl;
            glfwTerminate();
            return;
        }

        isFullscreen = true;
        width = mode->width;
        height = mode->height;

        // Assign context into this processing thread
        glfwMakeContextCurrent(window);

        // Init GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "ERROR: Cannot init GLAD" << std::endl;
            return;
        }

        // Lock the mouse cursor to the center of the screen and hide it
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Set the background color for the space environment
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);

        // Depth Testing
        glEnable(GL_DEPTH_TEST);

        // Preparing GPU data
        initShaders();
        sphere.initSphere(36, 18);
        quad.initQuad();
        postProcessor.init(width, height);
        initFramebuffers();

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
        shaderProgram = Shader("assets/shaders/planet.vert", "assets/shaders/planet.frag");
        screenShaderProgram = Shader("assets/shaders/screen.vert", "assets/shaders/screen.frag");
        blurShaderProgram = Shader("assets/shaders/screen.vert", "assets/shaders/blur.frag");
        skyboxShaderProgram = Shader("assets/shaders/skybox.vert", "assets/shaders/skybox.frag");
        shadowShaderProgram = Shader("assets/shaders/shadow.vert", "assets/shaders/shadow.frag", "assets/shaders/shadow.geom");
    }

    Renderer::~Renderer() {
        // Clean ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // Clean CPU resources before closing window
        sphere.cleanup();
        quad.cleanup();
        glDeleteFramebuffers(1, &depthMapFBO);
        glDeleteTextures(1, &depthCubemap);
        glDeleteFramebuffers(2, uiPingpongFBO);
        glDeleteTextures(2, uiPingpongColorbuffers);
        glDeleteProgram(shaderProgram.ID);
        glDeleteProgram(screenShaderProgram.ID);
        glDeleteProgram(blurShaderProgram.ID);
        glDeleteProgram(skyboxShaderProgram.ID);
        glDeleteProgram(shadowShaderProgram.ID);

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
        shadowShaderProgram.use();

        for (unsigned int i = 0; i < 6; ++i) {
            shadowShaderProgram.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        }

        shadowShaderProgram.setFloat("far_plane", far_plane);
        shadowShaderProgram.setVec3("lightPos", lightPos);

        // Drawing loop
        for (size_t i = 1; i < count; ++i) { // Exclude the Sun (i=0) because the Sun does not cast a shadow on itself
            glm::mat4 model = glm::mat4(1.0f);
            glm::vec3 pos((float)positions[i].x, (float)positions[i].y, (float)positions[i].z);
            float radius = (float)radii[i];
            model = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(orientations[i]) * glm::scale(glm::mat4(1.0f), glm::vec3(radius));

            shadowShaderProgram.setMat4("model", model);
            sphere.draw();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        
        // Render the 3D universe to the off-screen frame buffer
        postProcessor.beginRender();
        
        // Activate Shader Program
        shaderProgram.use();
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
        shaderProgram.setInt("depthMap", 2);
        shaderProgram.setFloat("far_plane", far_plane);

        // Setup CAMERA and SPACE

        // Projection Matrix
        // Creates a perspective effect (45 FOV, view range from 0.1 to 1000.0)
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
        shaderProgram.setMat4("projection", projection);

        // View Matrix
        glm::mat4 view = camera.getViewMatrix();
        shaderProgram.setMat4("view", view);

        // Send camera position to the atmospheric (Fresnel) shader
        shaderProgram.setVec3("viewPos", camera.Position);

        // // Send light source position (Fixed at the Star - position 0)
        if (count > 0) {
            shaderProgram.setVec3("lightPos", (float)positions[0].x, (float)positions[0].y, (float)positions[0].z);
        }

        // Draw SKYBOX
        glDepthFunc(GL_LEQUAL);
        skyboxShaderProgram.use();

        // Remove Translation part of Camera
        glm::mat4 viewNoTranslation = camera.getViewMatrixNoTranslation();

        // Inverse matrix
        skyboxShaderProgram.setMat4("invProjection", glm::inverse(projection));
        skyboxShaderProgram.setMat4("invView", glm::inverse(viewNoTranslation));

        quad.draw();

        glDepthFunc(GL_LESS);

        // Switch back to the main planet shader before drawing the bodies
        shaderProgram.use();

        // Drawing objects
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
            shaderProgram.setInt("bodyType", static_cast<int>(types[i]));
            shaderProgram.setFloat("temperature", (float)temperatures[i]);

            // Send this object's own model matrix to the GPU
            shaderProgram.setMat4("model", model);

            // Draw the sphere mesh
            sphere.draw();
        }

        // Apply Gaussian Blur for the UI frosted-glass background
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;

        glActiveTexture(GL_TEXTURE0);
        blurShaderProgram.use();
        blurShaderProgram.setInt("image", 0);
        float weights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };
        blurShaderProgram.setFloatArray("weight", weights, 5);
        for (unsigned int i = 0; i < amount; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, uiPingpongFBO[horizontal]);
            blurShaderProgram.setBool("horizontal", horizontal);

            // Get image from the scene color buffer
            glBindTexture(GL_TEXTURE_2D, first_iteration ? postProcessor.textureColorbuffer : uiPingpongColorbuffers[!horizontal]);

            quad.draw();

            horizontal = !horizontal;
            if (first_iteration) {
                first_iteration = false;
            }
        }

        // Apply Gaussian Blur for Bloom, then composite everything to the screen
        postProcessor.applyBlur(blurShaderProgram, quad);
        postProcessor.renderToScreen(screenShaderProgram, quad);
    }

    void Renderer::pollEvents() {
        glfwPollEvents();

        // Detect resolution changes (window drag, maximize, etc.)
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        if (fbWidth != width || fbHeight != height) {
            resize(fbWidth, fbHeight);
        }
    }

    void Renderer::resize(int newWidth, int newHeight) {
        if (newWidth <= 0 || newHeight <= 0) return; // Ignore minimized window
        if (newWidth == width && newHeight == height) return;

        width = newWidth;
        height = newHeight;

        // Re-allocate the off-screen scene/bloom buffers
        postProcessor.resize(width, height);

        // Re-allocate the UI frosted-glass blur buffers (textures stay attached)
        for (unsigned int i = 0; i < 2; i++) {
            glBindTexture(GL_TEXTURE_2D, uiPingpongColorbuffers[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Keep the OpenGL viewport in sync with the new size
        glViewport(0, 0, width, height);
    }

    void Renderer::processInput(float deltaTime) {
        // If escape button pressed -> close the window
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // F11 -> toggle fullscreen (edge-triggered so holding it doesn't flicker)
        bool f11Pressed = glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS;
        if (f11Pressed && !f11WasPressed) {
            if (!isFullscreen) {
                // Save the windowed geometry to restore it later
                glfwGetWindowPos(window, &windowedX, &windowedY);
                glfwGetWindowSize(window, &windowedWidth, &windowedHeight);

                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            }
            else {
                glfwSetWindowMonitor(window, nullptr, windowedX, windowedY,
                                     windowedWidth, windowedHeight, GLFW_DONT_CARE);
            }
            isFullscreen = !isFullscreen;
        }
        f11WasPressed = f11Pressed;

        // Camera movement and rotation (free-fly or orbit)
        camera.processInput(window, deltaTime);
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
        glm::mat4 view = camera.getViewMatrix();
        glm::vec3 ray_wor = glm::vec3(glm::inverse(view) * ray_eye);

        // Returns the normalized direction vector
        return glm::normalize(ray_wor);
    }

    void Renderer::initFramebuffers() {
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

        // Init Ping Pong FBOs for the UI frosted-glass blur
        glGenFramebuffers(2, uiPingpongFBO);
        glGenTextures(2, uiPingpongColorbuffers);
        for (unsigned int i = 0; i < 2; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, uiPingpongFBO[i]);
            glBindTexture(GL_TEXTURE_2D, uiPingpongColorbuffers[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, uiPingpongColorbuffers[i], 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Renderer::lockTarget(int entityIndex, float distance) {
        camera.lockTarget(entityIndex, distance);

        // When locking onto a target, hide the cursor to use the mouse for rotating the camera orbit
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void Renderer::unlockTarget() {
        camera.unlockTarget();
    }

    void Renderer::updateCameraTracking(const std::vector<Vector3>& positions) {
        camera.updateTracking(positions);
    }

    
}