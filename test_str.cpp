#include <iostream>
#include <string>

int main() {
    std::string n = "jupiter2_B.001";
    if (n.find("jupiter1") != std::string::npos) std::cout << "1\n";
    if (n.find("jupiter2") != std::string::npos) std::cout << "2\n";
    return 0;
}
