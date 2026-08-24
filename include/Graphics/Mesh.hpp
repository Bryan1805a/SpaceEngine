#pragma once
#include <glad/glad.h>
#include <vector>
#include <cmath>

namespace Graphics {
    class Mesh {
        public:
            unsigned int VAO, VBO, EBO;
            unsigned int indexCount;
            unsigned int vertexCount;
            bool hasIndices;

            Mesh();

            // Geometric generation functions
            void initSphere(int sectorCount, int stackCount);
            void initQuad();
            void initOrbitLine(int segments = 120);

            void draw(unsigned int mode = 0x0004) const;

            // Clean VRAM
            void cleanup();
    };
}