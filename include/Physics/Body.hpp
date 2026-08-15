#pragma once
#include <Math/Vector3.hpp>

namespace Physics {
    struct Body {
        double mass;
        Vector3 position;
        Vector3 velocity;
        Vector3 acceleration; // Save current a for Verlet algorithm

        // Default constructor
        Body(double m, const Vector3& pos, const Vector3& vel)
            : mass(m), position(pos), velocity(vel), acceleration(Vector3::Zero) {}
    };
    
}