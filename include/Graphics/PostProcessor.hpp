#pragma once
#include <glad/glad.h>
#include <Graphics/Shader.hpp>
#include <Graphics/Mesh.hpp>

namespace Graphics {
    class PostProcessor {
    public:
        unsigned int mainFBO;
        unsigned int textureColorbuffer;
        unsigned int textureBloombuffer;
        unsigned int RBO;

        unsigned int pingpongFBO[2];
        unsigned int pingpongColorbuffers[2];

        int width, height;

        // Default constructor (call init() before use)
        PostProcessor();

        // Constructor for initializing FBOs
        PostProcessor(int windowWidth, int windowHeight);

        // Clean VRAM
        ~PostProcessor();

        // Set up all FBOs for the given window size
        void init(int windowWidth, int windowHeight);

        // Enable rendering to the off-screen FBO
        void beginRender() const;

        // Run the blur algorithm for the luminance channel (Bloom Pass)
        void applyBlur(const Shader& blurShader, const Mesh& quadMesh, int amount = 15) const;

        // Blend images and output to screen
        void renderToScreen(const Shader& screenShader, const Mesh& quadMesh) const;

        // Update FBO dimensions when changing resolution
        void resize(int newWidth, int newHeight);

    private:
        mutable bool finalHorizontal; // Which pingpong buffer holds the final bloom blur

        void initFBOs();
        void cleanupFBOs();
    };
}
