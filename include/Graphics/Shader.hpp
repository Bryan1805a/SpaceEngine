#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

namespace Graphics {
    class Shader {
        public:
            unsigned int ID; // Program ID on VRAM

            // Default constructor (used so Renderer can hold Shader members)
            Shader() : ID(0) {}

            // Constructor reads file and compiles Shader
            Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);

            void use() const;

            void setBool(const std::string &name, bool value) const;
            void setInt(const std::string &name, int value) const;
            void setFloat(const std::string &name, float value) const;
            void setFloatArray(const std::string &name, const float* values, int count) const;
            void setVec3(const std::string &name, const glm::vec3 &value) const;
            void setVec3(const std::string &name, float x, float y, float z) const;
            void setMat4(const std::string &name, const glm::mat4 &mat) const;
    };
}