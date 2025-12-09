#pragma once

#include "types.hpp"
#include <vector>
#include <memory>
#include <array>

namespace hdmap {

// R-tree node for spatial indexing
// Using simplified R-tree with fixed capacity
constexpr size_t MAX_RTREE_ENTRIES = 8;
constexpr size_t MIN_RTREE_ENTRIES = 4;

enum class NodeType : uint8_t {
    LEAF,
    INTERNAL
};

// Entry in R-tree node
struct RTreeEntry {
    BoundingBox bbox;
    void* data;  // Points to either child node or map element
    
    RTreeEntry() : data(nullptr) {}
    RTreeEntry(const BoundingBox& bbox_, void* data_) : bbox(bbox_), data(data_) {}
};

class RTreeNode {
public:
    NodeType type;
    std::vector<RTreeEntry> entries;
    RTreeNode* parent;
    
    RTreeNode(NodeType type_) : type(type_), parent(nullptr) {
        entries.reserve(MAX_RTREE_ENTRIES);
    }
    
    bool isLeaf() const { return type == NodeType::LEAF; }
    bool isFull() const { return entries.size() >= MAX_RTREE_ENTRIES; }
    
    BoundingBox getBoundingBox() const;
};

// R-tree for efficient spatial queries
class RTree {
public:
    RTree();
    ~RTree();
    
    // Disable copy, enable move
    RTree(const RTree&) = delete;
    RTree& operator=(const RTree&) = delete;
    RTree(RTree&&) = default;
    RTree& operator=(RTree&&) = default;
    
    // Insert an element with its bounding box
    void insert(const BoundingBox& bbox, void* data);
    
    // Query elements within a bounding box
    void query(const BoundingBox& bbox, std::vector<void*>& results) const;
    
    // Query elements within radius of a point
    void queryRadius(const Point2D& center, double radius, std::vector<void*>& results) const;
    
    // Clear all entries
    void clear();
    
    // Get statistics
    size_t size() const { return elementCount_; }
    size_t height() const;
    
private:
    RTreeNode* root_;
    size_t elementCount_;
    
    // Helper methods
    RTreeNode* chooseLeaf(const BoundingBox& bbox);
    void splitNode(RTreeNode* node, RTreeEntry& newEntry);
    void adjustTree(RTreeNode* leaf);
    void queryNode(const RTreeNode* node, const BoundingBox& bbox, std::vector<void*>& results) const;
    double computeEnlargement(const BoundingBox& existing, const BoundingBox& addition) const;
    void deleteTree(RTreeNode* node);
};

} // namespace hdmap
