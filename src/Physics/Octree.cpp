#include <Physics/Octree.hpp>

namespace Physics {
    Octree::Octree(double rootSize, const std::vector<Vector3>& pos, const std::vector<double>& mass)
        : positions(pos), masses(mass) {
        // Pre-allocate memory for the tree to 
        // avoid performance degradation caused by frequent reallocations
        nodes.reserve(pos.size() * 4);
    }

    int Octree::createNode(const Vector3& center, double size) {
        nodes.emplace_back(center, size);
        return nodes.size() - 1;
    }

    int Octree::getOctant(const Vector3& nodeCenter, const Vector3& pos) const {
        int octant = 0;
        if (pos.x >= nodeCenter.x) octant |= 1;
        if (pos.y >= nodeCenter.y) octant |= 2;
        if (pos.z >= nodeCenter.z) octant |= 4;

        return octant;
    }

    void Octree::insertImpl(int nodeIdx, int bodyIdx) {
        OctreeNode& node = nodes[nodeIdx];
        Vector3 bodyPos = positions[bodyIdx];
        double bodyMass = masses[bodyIdx];

        // Update the total mass and center of mass 
        // for this node during the downward pass.
        double newTotalMass = node.totalMass + bodyMass;
        node.centerOfMass = (node.centerOfMass * node.totalMass + bodyPos + bodyMass) / newTotalMass;
        node.totalMass = newTotalMass;

        // If this node is Leaf
        if (node.isLeaf()) {
            // If it is empty
            if (node.bodyIndex == -1) {
                node.bodyIndex == bodyIdx;
                return;
            }

            // If it is not empty (Space collision)
            // Subdivide into 8
            double quarterSize = node.size / 4.0;
            for (int i = 0; i < OCTREE_CHILDREN; ++i) {
                // Calculate coordinates of centroid for each block
                double offsetX = ((i & 1) ? quarterSize : -quarterSize);
                double offsetY = ((i & 2) ? quarterSize : -quarterSize);
                double offsetZ = ((i & 4) ? quarterSize : -quarterSize);

                Vector3 childCenter = node.center + Vector3(offsetX, offsetY, offsetZ);
                node.children[i] = createNode(childCenter, node.size / 2.0);
            }

            // Move the old body down below
            int oldBodyIdx = node.bodyIndex;
            node.bodyIndex = -1; // This node become branch

            // Recursive to move the old body down
            int oldOctant = getOctant(node.center, positions[oldBodyIdx]);
            insertImpl(node.children[oldOctant], oldBodyIdx);
        }

        // If this node is a branch
        // Move the new body down to another block
        int targetOctant = getOctant(nodes[nodeIdx].center, bodyPos);
        insertImpl(nodes[nodeIdx].children[targetOctant], bodyIdx);
    }

    void Octree::build() {
        nodes.clear();
        // Initialize the universe with its center at (0, 0, 0) and an extremely large size
        rootIndex = createNode(Vector3::Zero, 2000.0);

        for (size_t i = 0; i < positions.size(); ++i) {
            insertImpl(rootIndex, i);
        }
    }
}