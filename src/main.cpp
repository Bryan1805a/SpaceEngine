#include <iostream>
#include <Math/vector3.hpp>

int main() {
    Vector3 a(1, 2, 3);
    Vector3 b(4, 5, 6);

    Vector3 c = a + b;
    Vector3 d = a - b;
    Vector3 e = a * 2.0;
    Vector3 f = 2.0 * a;

    std::cout << c << std::endl;
    std::cout << d << std::endl;
    std::cout << e << std::endl;
    std::cout << f << std::endl;

    std::cout << a.lengthSquared() << std::endl;
    std::cout << a.length() << std::endl;
    return 0;
}