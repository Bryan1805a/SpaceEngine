#include <iostream>
#include <cmath>
#include <Graphics/Mesh.hpp>
#include <../third_party/tiny_obj_loader.h>

namespace Graphics {
    Mesh::Mesh() : VAO(0), VBO(0), EBO(0), indexCount(0), vertexCount(0), hasIndices(false) {}

    void Mesh::initSphere(int sectorCount, int stackCount) {
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

        vertexCount = vertices.size() / 6;
        indexCount = indices.size();
        hasIndices = true;

        // Load data to VRAM (Including EBO)
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

    void Mesh::initQuad() {
        // Full-screen quad: (X, Y) position + (U, V) texture coordinate
        float quadVertices[] = {
            // Coordinates (X, Y)   // Texture Coordinate (U, V)
            -1.0f,  1.0f,           0.0f, 1.0f,
            -1.0f, -1.0f,           0.0f, 0.0f,
             1.0f, -1.0f,           1.0f, 0.0f,
            -1.0f,  1.0f,           0.0f, 1.0f,
             1.0f, -1.0f,           1.0f, 0.0f,
             1.0f,  1.0f,           1.0f, 1.0f
        };

        vertexCount = 6;
        indexCount = 0;
        hasIndices = false;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
    }

    void Mesh::draw(unsigned int mode) const {
        glBindVertexArray(VAO);
        if (hasIndices) {
            glDrawElements(mode, indexCount, GL_UNSIGNED_INT, 0);
        }
        else {
            glDrawArrays(mode, 0, vertexCount);
        }
    }

    void Mesh::drawSubMesh(size_t index, unsigned int mode) const {
        if (index >= subMeshes.size()) return;
        glBindVertexArray(VAO);
        if (hasIndices) {
            // Not implemented for indices yet
            glDrawElements(mode, indexCount, GL_UNSIGNED_INT, 0);
        }
        else {
            glDrawArrays(mode, subMeshes[index].vertexOffset, subMeshes[index].vertexCount);
        }
    }

    void Mesh::initOrbitLine(int segments) {
        std::vector<float> vertices;
        for (int i = 0; i < segments; ++i) {
            float theta = 2.0f * 3.14159265359f * float(i) / float(segments);
            vertices.push_back(std::cos(theta));
            vertices.push_back(0.0f);
            vertices.push_back(std::sin(theta));
        }

        vertexCount = segments;
        hasIndices = false;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Coordinates only (3 floats)
        // No normals or UVs
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FLOAT, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    void Mesh::cleanup() {
        if (hasIndices) {
            glDeleteBuffers(1, &EBO);
        }
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
    }

    bool Mesh::loadOBJ(const char* path) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        // Read .obj file
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path)) {
            std::cerr << "ERROR:TINYOBJ::" << warn << err << std::endl;
            return false;
        }

        std::vector<float> vertices;

        subMeshes.clear();

        // Scan across all the polygons of the model
        for (size_t s = 0; s < shapes.size(); s++) {
            SubMesh subMesh;
            subMesh.name = shapes[s].name;
            subMesh.vertexOffset = vertices.size() / 8;
            
            for (size_t i = 0; i < shapes[s].mesh.indices.size(); i++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[i];

                // Vertex coordinates (Position)
                vertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
                vertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
                vertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);

                // Normal vector
                if (idx.normal_index >= 0) {
                    vertices.push_back(attrib.normals[3 * idx.normal_index + 0]);
                    vertices.push_back(attrib.normals[3 * idx.normal_index + 1]);
                    vertices.push_back(attrib.normals[3 * idx.normal_index + 2]);
                }
                else {
                    float vx = attrib.vertices[3 * idx.vertex_index + 0];
                    float vy = attrib.vertices[3 * idx.vertex_index + 1];
                    float vz = attrib.vertices[3 * idx.vertex_index + 2];
                    float len = std::sqrt(vx*vx + vy*vy + vz*vz);
                    if (len > 1e-6f) {
                        vertices.push_back(vx / len);
                        vertices.push_back(vy / len);
                        vertices.push_back(vz / len);
                    } else {
                        vertices.push_back(0.0f);
                        vertices.push_back(1.0f);
                        vertices.push_back(0.0f);
                    }
                }

                // Image coordinates (UV/TexCoords)
                if (idx.texcoord_index >= 0) {
                    vertices.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
                    vertices.push_back(1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]); // Flip Y to match OpenGL
                }
                else {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                }
            }
            
            subMesh.vertexCount = (vertices.size() / 8) - subMesh.vertexOffset;
            subMeshes.push_back(subMesh);
        }

        // Normalize vertices to radius 1.0 to match the math/physics engine
        float maxRadius = 0.0f;
        for (size_t i = 0; i < vertices.size(); i += 8) {
            float r = std::sqrt(vertices[i]*vertices[i] + vertices[i+1]*vertices[i+1] + vertices[i+2]*vertices[i+2]);
            if (r > maxRadius) maxRadius = r;
        }
        if (maxRadius > 0.0001f) {
            for (size_t i = 0; i < vertices.size(); i += 8) {
                vertices[i] /= maxRadius;
                vertices[i+1] /= maxRadius;
                vertices[i+2] /= maxRadius;
            }
        }

        vertexCount = vertices.size() / 8; // (3 Pos + 3 Norm + 2 UV = 8 floats/vertex)
        hasIndices = false;

        // Push all data to VRAM
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // UV
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        return true;
    }
}
