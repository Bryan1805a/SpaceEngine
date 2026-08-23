#include <Physics/Octree.hpp>
#include <algorithm>

namespace Physics {
    Octree::Octree() : positions(nullptr), masses(nullptr), rootIndex(-1) {}

    Octree::Octree(const std::vector<Vector3>& pos, const std::vector<double>& mass)
        : positions(&pos), masses(&mass), rootIndex(-1) {
        // Pre-allocate memory for the tree to 
        // avoid performance degradation caused by frequent reallocations
        nodes.reserve(pos.size() * 4);
    }

    void Octree::bind(const std::vector<Vector3>& pos, const std::vector<double>& mass) {
        positions = &pos;
        masses = &mass;
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
        Vector3 bodyPos = (*positions)[bodyIdx];
        double bodyMass = (*masses)[bodyIdx];

        // Update the total mass and center of mass 
        // for this node during the downward pass.
        double newTotalMass = node.totalMass + bodyMass;
        node.centerOfMass = (node.centerOfMass * node.totalMass + bodyPos * bodyMass) / newTotalMass;
        node.totalMass = newTotalMass;

        // If this node is Leaf
        if (node.isLeaf()) {
            // If it is empty
            if (node.bodyIndex == -1) {
                node.bodyIndex = bodyIdx;
                return;
            }

            // If it is not empty (Space collision)
            // Subdivide into 8
            // Capture parent center/size before creating children,
            // because createNode() may reallocate the node vector and
            // invalidate any saved references.
            Vector3 parentCenter = node.center;
            double parentSize = node.size;
            double quarterSize = parentSize / 4.0;
            for (int i = 0; i < OCTREE_CHILDREN; ++i) {
                // Calculate coordinates of centroid for each block
                double offsetX = ((i & 1) ? quarterSize : -quarterSize);
                double offsetY = ((i & 2) ? quarterSize : -quarterSize);
                double offsetZ = ((i & 4) ? quarterSize : -quarterSize);

                Vector3 childCenter = parentCenter + Vector3(offsetX, offsetY, offsetZ);
                nodes[nodeIdx].children[i] = createNode(childCenter, parentSize / 2.0);
            }

            // Move the old body down below
            int oldBodyIdx = nodes[nodeIdx].bodyIndex;
            nodes[nodeIdx].bodyIndex = -1; // This node become branch

                // Recursive to move the old body down
            int oldOctant = getOctant(nodes[nodeIdx].center, (*positions)[oldBodyIdx]);
            insertImpl(nodes[nodeIdx].children[oldOctant], oldBodyIdx);
        }

        // If this node is a branch
        // Move the new body down to another block
        int targetOctant = getOctant(nodes[nodeIdx].center, bodyPos);
        insertImpl(nodes[nodeIdx].children[targetOctant], bodyIdx);
    }

    void Octree::build() {
        nodes.clear();
        if (positions->empty()) {
            rootIndex = createNode(Vector3::Zero, 1.0);
            return;
        }

        // Compute the bounding box of all bodies so the root adapts
        // to the actual simulation scale (galaxy, solar system, etc.)
        Vector3 min = (*positions)[0];
        Vector3 max = (*positions)[0];
        for (size_t i = 1; i < positions->size(); ++i) {
            const Vector3& p = (*positions)[i];
            min.x = std::min(min.x, p.x);
            min.y = std::min(min.y, p.y);
            min.z = std::min(min.z, p.z);
            max.x = std::max(max.x, p.x);
            max.y = std::max(max.y, p.y);
            max.z = std::max(max.z, p.z);
        }

        Vector3 center = (min + max) * 0.5;
        double extent = std::max({ max.x - min.x, max.y - min.y, max.z - min.z });
        // Enforce a minimum size for co-located bodies, plus a small margin
        double size = std::max(extent, 1.0) * 1.001;

        rootIndex = createNode(center, size);

        for (size_t i = 0; i < positions->size(); ++i) {
            insertImpl(rootIndex, i);
        }
    }

    Vector3 Octree::calculateAcceleration(int bodyIdx, double theta, double G) const {
        if (nodes.empty()) return Vector3::Zero;
        return calculateAccelImpl(rootIndex, bodyIdx, theta, G);
    }

    Vector3 Octree::calculateAccelImpl(int nodeIdx, int bodyIdx, double theta, double G) const {
        if (nodeIdx == -1) return Vector3::Zero;

        const OctreeNode& node = nodes[nodeIdx];
        Vector3 bodyPos = (*positions)[bodyIdx];

        // Rule 1: A planet cannot attract itself
        if (node.isLeaf() && node.bodyIndex == bodyIdx) {
            return Vector3::Zero;
        }

        // Calculate the distance from the planet to the center of mass of a spacial body
        Vector3 r_vec = node.centerOfMass - bodyPos;
        double r = r_vec.length();

        // Avoid divided by 0
        if (r == 0.0) return Vector3::Zero;

        // Rule 2: MAC - Multipole Acceptance Criterion
        // If the node is a leaf or the Size/Distance ratio is less than the Theta threshold
        // Then view this entire volume of space as a single, massive object
        if (node.isLeaf() || (node.size / r < theta)) {
            double r_cubeb = r * r * r;
            double accel_term = (G * node.totalMass) / r_cubeb;
            return r_vec * accel_term;
        }

        // Rule 3: Node is too close and too large, must go deeper into the smaller block
        Vector3 totalAccel = Vector3::Zero;
        for (int i = 0; i < OCTREE_CHILDREN; ++i) {
            if (node.children[i] != -1) {
                totalAccel += calculateAccelImpl(node.children[i], bodyIdx, theta, G);
            }
        }

        return totalAccel;
    }
}