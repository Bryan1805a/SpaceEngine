#include <iostream>
#include <cmath>
#include <Math/Vector3.hpp>

int main() {
    Vector3 a(3, 4, 0);
    Vector3 b(1e200, 1e200, 1e200);
    Vector3 c(1e-200, 1e-200, 1e-200);
    Vector3 d(0, 0, 0);

    std::cout << a.length() << std::endl;
    std::cout << b.length() << std::endl;
    std::cout << c.length() << std::endl;
    std::cout << d.length() << std::endl;
    return 0;
}