#pragma once
#include <Math/Vector3.hpp>
#include <vector>

namespace Physics {
    // Declare constants for 8 corners of the Octree
    const int OCTREE_CHILDREN = 8;

    struct OctreeNode{
        // Bounding Box
        Vector3 center; // Coordinates of the center of the cube
        double size; // The side length of the cube

        // Physics information for Barnes-Hut
        double totalMass; // Total mass of all planets inside
        Vector3 centerOfMass; // Coordinates of the centroid

        // Data management information
        int bodyIndex;
        int children[OCTREE_CHILDREN];

        // Constructor for creating empty node
        OctreeNode(const Vector3& c, double s) : center(c), size(s), totalMass(0.0), centerOfMass(Vector3::Zero), bodyIndex(-1) {
            for (int i = 0; i < OCTREE_CHILDREN; ++i) {
                children[i] = -1;
            }
        }

        // Check if this leaf or branch
        bool isLeaf() const {
            return children[0] == -1;
        } 
    };

    class Octree {
        private:
            std::vector<OctreeNode> nodes;
            const std::vector<Vector3>& positions; // Reference to Position array
            const std::vector<double>& masses;     // Reference tot Mass Array

            int rootIndex;

            // Creating a new Node and return its index
            int createNode(const Vector3& center, double size);

            // A function that determines which child node (0–7) a given position (pos) falls into
            int getOctant(const Vector3& nodeCenter, const Vector3& pos) const;

            // Recursive planet insertion function
            void insertImpl(int nodeIdx, int bodyIdx);

            // Recursive accelerating calculation function
            Vector3 calculateAccelImpl(int nodeIdx, int bodyIdx, double theta, double G) const;
        
        public:
            // Initialize a tree of cosmic scale and connect it to SoA data
            Octree(double rootSize, const std::vector<Vector3>& pos, const std::vector<double>& mass);

            // Rebuild the entire tree from scratch in every frame
            void build();

            Vector3 calculateAcceleration(int bodyIdx, double theta, double G) const;
    };
}