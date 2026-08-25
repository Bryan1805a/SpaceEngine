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
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <stb_image.h>
#include <iostream>

namespace Graphics {
    static unsigned int createColorTexture(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        unsigned char data[4] = {r, g, b, a};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return textureID;
    }

    // GLFW scroll callback: forwards mouse-wheel deltas to whichever Renderer owns
    // the window (looked up through the window user pointer, set in the ctor).
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        auto* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
        if (renderer) {
            renderer->addScroll((float)yoffset);
        }
    }

    Renderer::Renderer(int w, int h, const char* title) : width(w), height(h), window(nullptr), viewNear(0.1f), viewFar(1000.0f) {
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

        // Remember this Renderer for the scroll callback, then enable mouse-wheel zoom
        glfwSetWindowUserPointer(window, this);
        glfwSetScrollCallback(window, scrollCallback);

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

        // Load the HDR environment map used by the skybox
        hdrTexture = loadHDRTexture("assets/textures/skybox/HDR_multi_nebulae_1.hdr");

        // Load textures
        defaultSpecularMap = createColorTexture(25, 25, 25);
        defaultEmissionMap = createColorTexture(0, 0, 0);

        auto loadPlanet = [&](const char* objPath, const std::string& prefix) {
            PlanetAssets pa;
            pa.mesh.loadOBJ(objPath);
            for (auto& sm : pa.mesh.subMeshes) {
                SubMeshMaterial mat;
                mat.specularMap = defaultSpecularMap;
                mat.emissionMap = defaultEmissionMap;
                
                std::string albedoPath;
                std::string specPath;
                std::string emisPath;

                // Pattern matching for submeshes
                if (prefix == "mercury") {
                    albedoPath = "assets/textures/mercury_diffuse.png";
                } else if (prefix == "venus") {
                    albedoPath = "assets/textures/venus_surface.jpg";
                } else if (prefix == "earth") {
                    albedoPath = "assets/textures/earth_albedo.jpg";
                    specPath = "assets/textures/earth_land_ocean_mask.png";
                    emisPath = "assets/textures/earth_night_lights_modified.png";
                } else if (prefix == "mars") {
                    albedoPath = "assets/textures/mars_baseColor.png";
                    specPath = "assets/textures/mars_specularf0.png";
                } else if (prefix == "jupiter") {
                    if (sm.name.find("jupiter1") != std::string::npos) {
                        albedoPath = "assets/textures/Uv1_jupiter1_diff.png";
                        specPath = "assets/textures/Uv2_jupiter1_spec.png";
                    } else {
                        albedoPath = "assets/textures/Uv1_jupiter2_diff.png";
                    }
                } else if (prefix == "saturn") {
                    if (sm.name.find("saturn1") != std::string::npos || sm.name == "0") {
                        albedoPath = "assets/textures/saturn1_A_diffuse.png";
                        specPath = "assets/textures/saturn1_A_specularGlossiness.png";
                    } else if (sm.name.find("saturn2_B") != std::string::npos) {
                        albedoPath = "assets/textures/saturn2_B_diffuse.png";
                    } else if (sm.name.find("saturn2_A") != std::string::npos) {
                        albedoPath = "assets/textures/saturn2_A_diffuse.png";
                        specPath = "assets/textures/saturn2_A_specularGlossiness.png";
                    } else if (sm.name.find("Mimas") != std::string::npos) {
                        albedoPath = "assets/textures/saturn_Mimas_diffuse.png";
                    } else if (sm.name.find("Enceladus") != std::string::npos) {
                        albedoPath = "assets/textures/saturn_Enceladus_diffuse.png";
                    }
                } else if (prefix == "uranus") {
                    if (sm.name.find("uranus1") != std::string::npos || sm.name == "0") {
                        albedoPath = "assets/textures/uranus1_A_diffuse.png";
                        specPath = "assets/textures/uranus1_A_specularGlossiness.png";
                    } else if (sm.name.find("uranus2_B") != std::string::npos) {
                        albedoPath = "assets/textures/uranus2_B_diffuse.png";
                    } else if (sm.name.find("uranus2") != std::string::npos) {
                        albedoPath = "assets/textures/uranus2_A_diffuse.png";
                        specPath = "assets/textures/uranus2_A_specularGlossiness.png";
                    } else if (sm.name.find("miranda") != std::string::npos) {
                        albedoPath = "assets/textures/uranus_miranda_diffuse.png";
                    }
                } else if (prefix == "neptune") {
                    if (sm.name.find("neptune1") != std::string::npos) {
                        albedoPath = "assets/textures/neptune1_A_baseColor.png";
                        specPath = "assets/textures/neptune1_A_specularf0.png";
                    } else if (sm.name.find("neptune2_B") != std::string::npos) {
                        albedoPath = "assets/textures/neptune2_B_baseColor.png";
                    } else {
                        albedoPath = "assets/textures/neptune2_A_baseColor.png";
                    }
                } else if (prefix == "moon") {
                    albedoPath = "assets/textures/moon_baseColor.png";
                    specPath = "assets/textures/moon_specularf0.png";
                }

                if (!albedoPath.empty()) mat.albedoMap = loadTexture(albedoPath.c_str());
                if (!specPath.empty()) mat.specularMap = loadTexture(specPath.c_str());
                if (!emisPath.empty()) mat.emissionMap = loadTexture(emisPath.c_str());

                pa.materials.push_back(mat);
            }
            return pa;
        };

        // Note: The order must match main.cpp Simulation::addBody
        // Sun(0), Mercury(1), Venus(2), Earth(3), Mars(4), Jupiter(5), Saturn(6), Uranus(7), Neptune(8), Moon(9)
        planetAssets.push_back(PlanetAssets()); // Sun uses sphere
        planetAssets.push_back(loadPlanet("assets/models/mercury.obj", "mercury"));
        planetAssets.push_back(loadPlanet("assets/models/venus.obj", "venus"));
        planetAssets.push_back(loadPlanet("assets/models/earth.obj", "earth"));
        planetAssets.push_back(loadPlanet("assets/models/mars.obj", "mars"));
        planetAssets.push_back(loadPlanet("assets/models/jupiter.obj", "jupiter"));
        planetAssets.push_back(loadPlanet("assets/models/saturn.obj", "saturn"));
        planetAssets.push_back(loadPlanet("assets/models/uranus.obj", "uranus"));
        planetAssets.push_back(loadPlanet("assets/models/neptune.obj", "neptune"));
        planetAssets.push_back(loadPlanet("assets/models/moon.obj", "moon"));


        // Init ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        // Interface customization
        // Modern monochrome "floating window" theme: rounded corners, translucent
        // white glass, hairline white borders, and no color accents.
        ImGui::StyleColorsLight();
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding    = 12.0f;
        style.ChildRounding     = 10.0f;
        style.FrameRounding     = 8.0f;
        style.PopupRounding     = 10.0f;
        style.ScrollbarRounding = 8.0f;
        style.GrabRounding      = 8.0f;
        style.TabRounding       = 8.0f;

        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize  = 1.0f;
        style.FrameBorderSize  = 0.0f;
        style.PopupBorderSize  = 1.0f;

        style.WindowPadding    = ImVec2(14, 14);
        style.FramePadding     = ImVec2(10, 6);
        style.ItemSpacing      = ImVec2(10, 8);
        style.ItemInnerSpacing = ImVec2(8, 6);
        style.ScrollbarSize    = 12.0f;
        style.GrabMinSize      = 10.0f;
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

        // Monochrome palette: pure white accents over transparent glass.
        style.Colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
        style.Colors[ImGuiCol_TextDisabled]          = ImVec4(1.00f, 1.00f, 1.00f, 0.40f);
        style.Colors[ImGuiCol_WindowBg]              = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        style.Colors[ImGuiCol_ChildBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
        style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.04f, 0.04f, 0.05f, 0.90f);
        style.Colors[ImGuiCol_Border]                = ImVec4(1.00f, 1.00f, 1.00f, 0.22f);
        style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_FrameBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.08f);
        style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(1.00f, 1.00f, 1.00f, 0.15f);
        style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(1.00f, 1.00f, 1.00f, 0.22f);
        style.Colors[ImGuiCol_TitleBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.08f);
        style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(1.00f, 1.00f, 1.00f, 0.14f);
        style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(1.00f, 1.00f, 1.00f, 0.25f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(1.00f, 1.00f, 1.00f, 0.40f);
        style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(1.00f, 1.00f, 1.00f, 0.55f);
        style.Colors[ImGuiCol_CheckMark]             = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
        style.Colors[ImGuiCol_SliderGrab]            = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
        style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_Button]                = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
        style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(1.00f, 1.00f, 1.00f, 0.18f);
        style.Colors[ImGuiCol_ButtonActive]          = ImVec4(1.00f, 1.00f, 1.00f, 0.28f);
        style.Colors[ImGuiCol_Header]                = ImVec4(1.00f, 1.00f, 1.00f, 0.08f);
        style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
        style.Colors[ImGuiCol_HeaderActive]          = ImVec4(1.00f, 1.00f, 1.00f, 0.26f);
        style.Colors[ImGuiCol_Separator]             = ImVec4(1.00f, 1.00f, 1.00f, 0.14f);
        style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(1.00f, 1.00f, 1.00f, 0.26f);
        style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(1.00f, 1.00f, 1.00f, 0.42f);
        style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
        style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.00f, 1.00f, 1.00f, 0.40f);
        style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
        style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(1.00f, 1.00f, 1.00f, 0.08f);
        style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(1.00f, 1.00f, 1.00f, 0.22f);
        style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
        style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
        style.Colors[ImGuiCol_Tab]                   = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        style.Colors[ImGuiCol_TabHovered]            = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
        style.Colors[ImGuiCol_TabSelected]           = ImVec4(1.00f, 1.00f, 1.00f, 0.22f);
        style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(1.00f, 1.00f, 1.00f, 0.22f);
        style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(1.00f, 1.00f, 1.00f, 0.28f);
        style.Colors[ImGuiCol_NavHighlight]          = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);

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
        flareShaderProgram = Shader("assets/shaders/screen.vert", "assets/shaders/flare.frag");
        orbitShader = Shader("assets/shaders/orbit.vert", "assets/shaders/orbit.frag");
        orbitMesh.initOrbitLine(120);
    }

    Renderer::~Renderer() {
        // Clean ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // Clean CPU resources before closing window
        sphere.cleanup();
        quad.cleanup();
        orbitMesh.cleanup();
        glDeleteFramebuffers(1, &depthMapFBO);
        glDeleteTextures(1, &depthCubemap);
        glDeleteFramebuffers(2, uiPingpongFBO);
        glDeleteTextures(2, uiPingpongColorbuffers);
        glDeleteTextures(1, &hdrTexture);

        // Release the per-planet meshes and material textures (the default maps
        // are shared across materials, but glDeleteTextures is idempotent).
        for (auto& pa : planetAssets) {
            pa.mesh.cleanup();
            for (auto& m : pa.materials) {
                glDeleteTextures(1, &m.albedoMap);
                glDeleteTextures(1, &m.specularMap);
                glDeleteTextures(1, &m.emissionMap);
            }
        }
        glDeleteTextures(1, &defaultSpecularMap);
        glDeleteTextures(1, &defaultEmissionMap);
        glDeleteProgram(shaderProgram.ID);
        glDeleteProgram(screenShaderProgram.ID);
        glDeleteProgram(blurShaderProgram.ID);
        glDeleteProgram(skyboxShaderProgram.ID);
        glDeleteProgram(shadowShaderProgram.ID);
        glDeleteProgram(flareShaderProgram.ID);
        glDeleteProgram(orbitShader.ID);

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

    void Renderer::draw(size_t count, const std::vector<Vector3>& positions, const std::vector<Vector3>& velocities, const std::vector<double>& radii, const std::vector<glm::quat>& orientations, const std::vector<Simulation::BodyType>& types, const std::vector<double>& temperatures, const std::vector<int>& assetIndices, const std::vector<int>& parentIds, const std::vector<int>& ids) const {
        // Calculate 6 matrices from the Sun's perspective (90-degree field of view)
        float near_plane = 1.0f;
        float far_plane = 250.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);

        // Locate the light source by type rather than assuming it is body 0, so
        // lighting/shadow/flare stay correct after bodies are added or removed.
        int sunIndex = -1;
        for (size_t i = 0; i < count; ++i) {
            if (types[i] == Simulation::BodyType::STAR) {
                sunIndex = (int)i;
                break;
            }
        }

        glm::vec3 lightPos(0.0f);
        if (sunIndex >= 0) {
            lightPos = glm::vec3((float)positions[sunIndex].x, (float)positions[sunIndex].y, (float)positions[sunIndex].z);
        }

        // Resolve a body index to its model assets, keyed by the stable asset
        // index carried with each body (not the transient array position).
        auto selectAssets = [&](size_t i) -> const PlanetAssets* {
            int a = (i < assetIndices.size()) ? assetIndices[i] : -1;
            if (a >= 0 && a < (int)planetAssets.size() && planetAssets[a].mesh.vertexCount > 0) {
                return &planetAssets[a];
            }
            return nullptr;
        };

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
        for (size_t i = 0; i < count; ++i) { // Exclude the Sun because it does not cast a shadow on itself
            if ((int)i == sunIndex) continue;
            glm::mat4 model = glm::mat4(1.0f);
            glm::vec3 pos((float)positions[i].x, (float)positions[i].y, (float)positions[i].z);
            float radius = (float)radii[i];
            model = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(orientations[i]) * glm::scale(glm::mat4(1.0f), glm::vec3(radius));

            shadowShaderProgram.setMat4("model", model);

            const PlanetAssets* pa = selectAssets(i);
            if (pa) {
                pa->mesh.draw();
            } else {
                sphere.draw();
            }
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

        // Adaptive near/far planes, derived from the distance to the nearest scene
        // content. This keeps the depth buffer precise whether we're viewing the
        // whole solar system (~100s of units away) or a single planet (~1e-4 units).
        float nearestScene = computeNearestSceneDistance(count, positions, radii);

        // Near plane: a small fraction of the nearest content so nothing the camera
        // is looking at gets clipped. Far plane: enough to cover the whole system.
        viewNear = glm::clamp(nearestScene * 0.01f, 5.0e-6f, 0.5f);
        viewFar = glm::max(nearestScene * 500.0f, 100.0f);

        // Projection Matrix
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, viewNear, viewFar);
        shaderProgram.setMat4("projection", projection);

        // View Matrix
        glm::mat4 view = camera.getViewMatrix();
        shaderProgram.setMat4("view", view);

        // Send camera position to the atmospheric (Fresnel) shader
        shaderProgram.setVec3("viewPos", camera.Position);

        // Send light source position (the host star, if any)
        shaderProgram.setVec3("lightPos", lightPos.x, lightPos.y, lightPos.z);

        // Draw SKYBOX
        glDepthFunc(GL_LEQUAL);
        skyboxShaderProgram.use();

        // Bind the equirectangular HDR environment map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        skyboxShaderProgram.setInt("skybox", 0);

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

            const PlanetAssets* pa = selectAssets(i);
            if (pa) {
                shaderProgram.setInt("isEarth", 1); // Triggers texture logic
                for (size_t sm = 0; sm < pa->mesh.subMeshes.size(); ++sm) {
                    const auto& mat = pa->materials[sm];
                    
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, mat.albedoMap);
                    shaderProgram.setInt("albedoMap", 3);
                    
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_2D, mat.specularMap);
                    shaderProgram.setInt("specularMap", 4);

                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, mat.emissionMap);
                    shaderProgram.setInt("emissionMap", 5);

                    pa->mesh.drawSubMesh(sm);
                }
            } else {
                shaderProgram.setInt("isEarth", 0);
                sphere.draw();
            }
        }

        // Draw Orbits
        if (count > 0) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            orbitShader.use();
            orbitShader.setMat4("projection", projection);
            orbitShader.setMat4("view", view);

            for (size_t i = 0; i < count; ++i) {
                if ((int)i == sunIndex) continue; // The star itself has no orbit line

                // Resolve the orbit parent: default to the host star, otherwise
                // follow the stable parent id recorded with the body.
                int parentIdx = sunIndex;
                int pid = (i < parentIds.size()) ? parentIds[i] : -1;
                if (pid != -1) {
                    parentIdx = -1;
                    for (size_t k = 0; k < ids.size(); ++k) {
                        if (ids[k] == pid) {
                            parentIdx = (int)k;
                            break;
                        }
                    }
                }
                if (parentIdx < 0 || parentIdx >= (int)count) continue; // Parent removed

                glm::vec3 parentPos((float)positions[parentIdx].x, (float)positions[parentIdx].y, (float)positions[parentIdx].z);
                glm::vec3 pos((float)positions[i].x, (float)positions[i].y, (float)positions[i].z);
                
                glm::vec3 r = pos - parentPos;
                float dist = glm::length(r);

                glm::vec3 parentVel((float)velocities[parentIdx].x, (float)velocities[parentIdx].y, (float)velocities[parentIdx].z);
                glm::vec3 vel((float)velocities[i].x, (float)velocities[i].y, (float)velocities[i].z);
                glm::vec3 v = vel - parentVel;

                // Angular momentum vector h = r x v is perpendicular to the orbital plane
                glm::vec3 h = glm::cross(r, v);
                if (glm::length(h) > 1e-6f) {
                    h = glm::normalize(h);
                } else {
                    h = glm::vec3(0.0f, 1.0f, 0.0f); // Fallback
                }

                // Default orbit is in X-Z plane, so its normal is +Y. Rotate +Y to match h.
                glm::vec3 up(0.0f, 1.0f, 0.0f);
                glm::quat q = glm::rotation(up, h);
                glm::mat4 rot = glm::mat4_cast(q);

                glm::mat4 model = glm::translate(glm::mat4(1.0f), parentPos) * rot * glm::scale(glm::mat4(1.0f), glm::vec3(dist));
                orbitShader.setMat4("model", model);
                
                // Color
                glm::vec4 orbitColor(0.5f, 0.5f, 0.5f, 0.25f);
                
                orbitShader.setVec4("orbitColor", orbitColor);
                orbitMesh.draw(GL_LINE_LOOP);
            }
            glDisable(GL_BLEND);
        }

        // Draw Lens Flare on top of the scene (additive, screen-space)
        drawLensFlare(count, positions, radii, sunIndex);

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

        // Tab -> toggle the persistent UI cursor mode (show cursor, pause mouse-look)
        bool tabPressed = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
        if (tabPressed && !tabWasPressed) {
            uiCursorEnabled = !uiCursorEnabled;
            camera.setUIMode(uiCursorEnabled);
        }
        tabWasPressed = tabPressed;

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
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, viewNear, viewFar);
        glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
        ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f); // Set w=0 to turn it into a direction vector

        // View Space to World Space
        glm::mat4 view = camera.getViewMatrix();
        glm::vec3 ray_wor = glm::vec3(glm::inverse(view) * ray_eye);

        // Returns the normalized direction vector
        return glm::normalize(ray_wor);
    }

    bool Renderer::worldToScreen(const glm::vec3& worldPos, glm::vec2& outScreenPos, float& outDist) const {
        // Rebuild the same projection/view used by the main draw pass
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, viewNear, viewFar);
        glm::mat4 view = camera.getViewMatrix();

        glm::vec4 clip = projection * view * glm::vec4(worldPos, 1.0f);

        // Behind the camera (or on the near plane) -> not visible
        if (clip.w <= 0.0f) return false;

        glm::vec3 ndc = glm::vec3(clip) / clip.w;

        // Outside the frustum (|x| or |y| > 1, or z outside [-1, 1])
        if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f || ndc.z < -1.0f || ndc.z > 1.0f) {
            return false;
        }

        // Convert NDC to pixel coordinates (flip Y because ImGui origin is top-left)
        outScreenPos.x = (ndc.x * 0.5f + 0.5f) * (float)width;
        outScreenPos.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * (float)height;

        // Distance from the camera (in world units)
        outDist = glm::length(worldPos - camera.Position);
        return true;
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

    void Renderer::lockTarget(int entityIndex, float distance, float planetRadius) {
        camera.lockTarget(entityIndex, distance, planetRadius);

        // Locking a target puts us back into space mode (hide cursor, enable look)
        uiCursorEnabled = false;
        camera.setUIMode(false);

        // When locking onto a target, hide the cursor to use the mouse for rotating the camera orbit
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void Renderer::unlockTarget() {
        camera.unlockTarget();
    }

    float Renderer::computeNearestSceneDistance(size_t count, const std::vector<Vector3>& positions, const std::vector<double>& radii) const {
        if (count == 0) {
            return 20.0f;
        }

        glm::vec3 camPos = camera.Position;
        float nearest = 1.0e10f;

        for (size_t i = 0; i < count; ++i) {
            glm::vec3 body((float)positions[i].x, (float)positions[i].y, (float)positions[i].z);
            float radius = (float)radii[i];

            // Distance from the camera to the body's surface, never below 0.
            float surfaceDist = glm::length(camPos - body) - radius;
            if (surfaceDist < nearest) {
                nearest = surfaceDist;
            }
        }

        // Keep a healthy minimum so the view never collapses onto a single point.
        if (nearest < 1.0e-5f) nearest = 1.0e-5f;
        return nearest;
    }

    void Renderer::updateCameraTracking(const std::vector<Vector3>& positions) {
        camera.updateTracking(positions);
    }

    void Renderer::drawLensFlare(size_t count, const std::vector<Vector3>& positions, const std::vector<double>& radii, int sunIndex) const {
        if (count == 0 || sunIndex < 0 || sunIndex >= (int)count) return;

        // The host star projects a flare. Locate it by the passed index (resolved
        // from body type in draw()), not by assuming it is body 0.
        glm::vec3 sunPos((float)positions[sunIndex].x, (float)positions[sunIndex].y, (float)positions[sunIndex].z);

        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, viewNear, viewFar);
        glm::mat4 view = camera.getViewMatrix();

        glm::vec4 clip = proj * view * glm::vec4(sunPos, 1.0f);

        // Behind the camera -> no flare.
        float sunVisible = 1.0f;
        if (clip.w <= 0.0f) {
            sunVisible = 0.0f;
        } else {
            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            // If the Sun is off-screen we still allow a faint flare from its glow
            // when it's just outside the frame, but fade quickly beyond the edge.
            float edge = glm::max(glm::max(abs(ndc.x), abs(ndc.y)), 1.0f);
            sunVisible = glm::clamp(edge <= 1.0f ? 1.0f : 1.0f / (1.0f + (edge - 1.0f) * 6.0f), 0.0f, 1.0f);

            // Occlusion test: if any planet's sphere blocks the ray Camera->Sun,
            // the flare is hidden (the planets are tiny, so this is subtle).
            glm::vec3 camPos = camera.Position;
            glm::vec3 toSun = sunPos - camPos;
            float sunDist = glm::length(toSun);
            glm::vec3 rayDir = sunDist > 1.0e-6f ? toSun / sunDist : glm::vec3(0.0f);
            for (size_t i = 0; i < count; ++i) {
                if ((int)i == sunIndex) continue;
                glm::vec3 center((float)positions[i].x, (float)positions[i].y, (float)positions[i].z);
                float radius = (float)radii[i];
                glm::vec3 oc = center - camPos;
                float b = glm::dot(oc, rayDir);
                if (b <= 0.0f || b >= sunDist) continue;          // behind camera or past the Sun
                float c = glm::dot(oc, oc) - radius * radius;
                float disc = b * b - c;
                if (disc > 0.0f) {                                 // ray passes through the sphere
                    float t = b - std::sqrt(disc);
                    if (t > 0.0f && t < sunDist) {
                        sunVisible = 0.0f;
                        break;
                    }
                }
            }
        }

        // Position in UV space (flip Y so it matches the screen's TexCoords).
        float u = (clip.x / clip.w) * 0.5f + 0.5f;
        float v = (clip.y / clip.w) * 0.5f + 0.5f;

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);  // additive
        glDepthMask(GL_FALSE);        // flare never writes depth
        glDisable(GL_DEPTH_TEST);     // flare always draws on top; occlusion is CPU-side

        flareShaderProgram.use();
        flareShaderProgram.setVec2("sunPos", u, v);
        flareShaderProgram.setFloat("sunVisible", sunVisible);
        flareShaderProgram.setFloat("aspect", (float)width / (float)height);
        
        // Calculate the Sun's size in UV space based on perspective projection
        float sunWorldRadius = (float)radii[sunIndex];
        glm::vec3 toSun = sunPos - camera.Position;
        float fov = glm::radians(45.0f);
        float sunRadiusNDC = (sunWorldRadius / std::max(glm::length(toSun), 1.0e-6f)) / std::tan(fov * 0.5f);
        float baseRadius = sunRadiusNDC * 0.5f; 
        flareShaderProgram.setFloat("baseRadius", baseRadius);

        // Boost so the halo reads clearly against the dark space background.
        flareShaderProgram.setFloat("intensity", 1.0f);

        quad.draw();

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST); // restore
        glDisable(GL_BLEND);
    }

    unsigned int Renderer::loadHDRTexture(const char* path) {
        // Flip
        stbi_set_flip_vertically_on_load(true);
        
        int width, height, nrComponents;
        float *data = stbi_loadf(path, &width, &height, &nrComponents, 0);
        
        unsigned int hdrTexture = 0;
        if (data) {
            glGenTextures(1, &hdrTexture);
            glBindTexture(GL_TEXTURE_2D, hdrTexture);
            
            // Force the GL_RGB16F (16-bit Float) format to preserve HDR brightness values ​​greater than 1.0
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); 
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            stbi_image_free(data);
        } else {
            std::cerr << "ERROR::HDR::Cannot download the image: " << path << std::endl;
        }
        return hdrTexture;
    }

    unsigned int Renderer::loadTexture(const char* path) {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        stbi_set_flip_vertically_on_load(false);
        unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);

        if (data) {
            GLenum format = GL_RGBA;
            bool needAlignmentFix = false;

            if (nrComponents == 1) {
                format = GL_RED;
                needAlignmentFix = true;
            }
            else if (nrComponents == 2) {
                format = GL_RG;
                needAlignmentFix = true;
            }
            else if (nrComponents == 3) {
                format = GL_RGB;
                needAlignmentFix = true;
            }
            else if (nrComponents == 4) {
                format = GL_RGBA;
            }

            if (needAlignmentFix) {
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            }

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D); // Generate mipmaps to prevent noise in downscaled images

            if (needAlignmentFix) {
                glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // Restore default
            }

            // For single-channel textures (e.g. grayscale masks), set the swizzle
            // so that sampling .rgb returns (R,R,R) instead of (R,0,0).
            if (nrComponents == 1) {
                GLint swizzle[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
                glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else {
            std::cerr << "ERROR::TEXTURE::Cannot load image: " << path << std::endl;
        }

        return textureID;
    }
}