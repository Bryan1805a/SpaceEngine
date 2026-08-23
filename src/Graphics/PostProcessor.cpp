#include <Graphics/PostProcessor.hpp>

namespace Graphics {
    PostProcessor::PostProcessor()
        : mainFBO(0), textureColorbuffer(0), textureBloombuffer(0), RBO(0),
          width(0), height(0), finalHorizontal(true) {
        pingpongFBO[0] = pingpongFBO[1] = 0;
        pingpongColorbuffers[0] = pingpongColorbuffers[1] = 0;
    }

    PostProcessor::PostProcessor(int windowWidth, int windowHeight) : PostProcessor() {
        init(windowWidth, windowHeight);
    }

    PostProcessor::~PostProcessor() {
        cleanupFBOs();
    }

    void PostProcessor::init(int windowWidth, int windowHeight) {
        width = windowWidth;
        height = windowHeight;

        initFBOs();
    }

    void PostProcessor::initFBOs() {
        // Main off-screen FBO: scene color + bloom highlight + depth/stencil
        glGenFramebuffers(1, &mainFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, mainFBO);

        // Ordinary scenery color texture
        glGenTextures(1, &textureColorbuffer);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

        // Bloom highlight texture (contains only the bright areas)
        glGenTextures(1, &textureBloombuffer);
        glBindTexture(GL_TEXTURE_2D, textureBloombuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, textureBloombuffer, 0);

        unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);

        // Depth/Stencil renderbuffer
        glGenRenderbuffers(1, &RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Ping-Pong FBOs for the bloom blur
        glGenFramebuffers(2, pingpongFBO);
        glGenTextures(2, pingpongColorbuffers);
        for (unsigned int i = 0; i < 2; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
            glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void PostProcessor::cleanupFBOs() {
        glDeleteFramebuffers(2, pingpongFBO);
        glDeleteTextures(2, pingpongColorbuffers);
        glDeleteTextures(1, &textureColorbuffer);
        glDeleteTextures(1, &textureBloombuffer);
        glDeleteRenderbuffers(1, &RBO);
        glDeleteFramebuffers(1, &mainFBO);

        mainFBO = textureColorbuffer = textureBloombuffer = RBO = 0;
        pingpongFBO[0] = pingpongFBO[1] = 0;
        pingpongColorbuffers[0] = pingpongColorbuffers[1] = 0;
    }

    void PostProcessor::resize(int newWidth, int newHeight) {
        if (newWidth <= 0 || newHeight <= 0) return; // Ignore minimize events
        if (newWidth == width && newHeight == height) return; // Nothing to do

        cleanupFBOs();
        width = newWidth;
        height = newHeight;
        initFBOs();
    }

    void PostProcessor::beginRender() const {
        glBindFramebuffer(GL_FRAMEBUFFER, mainFBO);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void PostProcessor::applyBlur(const Shader& blurShader, const Mesh& quadMesh, int amount) const {
        bool horizontal = true, first_iteration = true;

        glActiveTexture(GL_TEXTURE0);
        blurShader.use();
        blurShader.setInt("image", 0);
        float weights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };
        blurShader.setFloatArray("weight", weights, 5);

        for (int i = 0; i < amount; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            blurShader.setBool("horizontal", horizontal);

            // First iteration samples the bloom highlight, then ping-pongs between buffers
            glBindTexture(GL_TEXTURE_2D, first_iteration ? textureBloombuffer : pingpongColorbuffers[!horizontal]);

            quadMesh.draw();

            horizontal = !horizontal;
            if (first_iteration) {
                first_iteration = false;
            }
        }

        // Remember which buffer holds the final result for the screen pass
        finalHorizontal = horizontal;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void PostProcessor::renderToScreen(const Shader& screenShader, const Mesh& quadMesh) const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        screenShader.use();

        // Scene color to texture slot 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        screenShader.setInt("screenTexture", 0);

        // Bloom blur to texture slot 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!finalHorizontal]);
        screenShader.setInt("bloomBlur", 1);

        quadMesh.draw();

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }
}
