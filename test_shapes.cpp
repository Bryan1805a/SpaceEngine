#include <iostream>
#define TINYOBJLOADER_DISABLE_FAST_FLOAT
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

void printShapes(const char* path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path);
    std::cout << "--- " << path << " ---" << std::endl;
    for (size_t s = 0; s < shapes.size(); s++) {
        std::cout << "Shape " << s << " name: " << shapes[s].name << std::endl;
    }
}

int main() {
    printShapes("assets/models/venus.obj");
    printShapes("assets/models/uranus.obj");
    return 0;
}
