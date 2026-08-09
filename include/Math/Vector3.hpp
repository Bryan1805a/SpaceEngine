#pragma once
#include <cmath>
#include <ostream>

struct Vector3 {
    double x, y, z;

    // Default constructor
    Vector3(double x = 0.0, double y = 0.0, double z = 0.0)
        : x(x), y(y), z(z) {}

    // Constant
    static const Vector3 Zero;
    static const Vector3 One;
    static const Vector3 UnitX;
    static const Vector3 UnitY;
    static const Vector3 UnitZ;

    // Basic operations
    // Addition: a + b
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    // Addition with assignment: a += b
    Vector3& operator+=(const Vector3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    // Substraction: a - b
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    // Substraction with asignment: a -= b
    Vector3& operator-=(const Vector3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    // Unary minus: -a
    Vector3 operator-() const {
        return Vector3(-x, -y, -z);
    }

    // Scalar operation
    // Scalar multiplication: a * scalar
    Vector3 operator*(double scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    // Scalar multiplication with asignment: a *= scalar
    Vector3& operator*=(double scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    // Scalar division: a / scalar
    Vector3 operator/(double scalar) const {
        return Vector3(x / scalar, y / scalar, z / scalar);
    }

    // Scalar division with asignment: a /= scalar
    Vector3& operator/=(double scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    // Length Operations
    // Length Squared: a**2
    double lengthSquared() const {
        return x * x + y * y + z * z;
    }

    // Length: a
    double length() const {
        return std::sqrt(lengthSquared());
    }

    // Comparision Operators
    bool operator==(const Vector3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const Vector3& other) const {
        return !(*this == other);
    }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const Vector3& v) {
        os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }
};

inline const Vector3 Vector3::Zero = Vector3(0.0, 0.0, 0.0);
inline const Vector3 Vector3::One = Vector3(1.0, 1.0, 1.0);
inline const Vector3 Vector3::UnitX = Vector3(1.0, 0.0, 0.0);
inline const Vector3 Vector3::UnitY = Vector3(0.0, 1.0, 0.0);
inline const Vector3 Vector3::UnitZ = Vector3(0.0, 0.0, 1.0);

// Free functions
// Scalar multiplication
Vector3 operator*(double scalar, const Vector3& v) {
    return v * scalar;
}