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
    if(shapes.size() > 0)
        std::cout << path << " Shape 0 name: " << shapes[0].name << std::endl;
}

int main() {
    printShapes("assets/models/mars.obj");
    printShapes("assets/models/mercury.obj");
    printShapes("assets/models/neptune.obj");
    printShapes("assets/models/moon.obj");
    printShapes("assets/models/earth.obj");
    return 0;
}
