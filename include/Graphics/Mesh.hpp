#pragma once
#include <glad/glad.h>
#include <vector>
#include <cmath>

namespace Graphics {
    struct SubMesh {
        std::string name;
        unsigned int vertexOffset;
        unsigned int vertexCount;
    };

    class Mesh {
        public:
            unsigned int VAO, VBO, EBO;
            unsigned int indexCount;
            unsigned int vertexCount;
            bool hasIndices;
            
            std::vector<SubMesh> subMeshes;

            Mesh();

            // Geometric generation functions
            void initSphere(int sectorCount, int stackCount);
            void initQuad();
            void initOrbitLine(int segments = 120);
            bool loadOBJ(const char* path);

            void draw(unsigned int mode = 0x0004) const;
            void drawSubMesh(size_t index, unsigned int mode = 0x0004) const;

            // Clean VRAM
            void cleanup();
    };
}