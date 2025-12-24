#pragma once

#include <array>
#include <memory>
#include <vector>
#include <variant>

#include "types.hpp"

namespace hdmap {

// R-tree node for spatial indexing
// Using simplified R-tree with fixed capacity
constexpr size_t MAX_RTREE_ENTRIES = 8;
constexpr size_t MIN_RTREE_ENTRIES = 4;

enum class NodeType : uint8_t { LEAF, INTERNAL };

class RTreeNode;
using Data = std::variant<std::shared_ptr<RTreeNode>, std::shared_ptr<Lane>, std::shared_ptr<TrafficLight>, std::shared_ptr<TrafficSign>>;

// Entry in R-tree node
struct RTreeEntry {
  BoundingBox bbox;
  Data data;  // Points to either child node or map element

  RTreeEntry() : data{} {
  }
  RTreeEntry(const BoundingBox& bbox, const Data data)
      : bbox{bbox}, data{data} {
  }
};

class RTreeNode {
 public:
  NodeType type;
  std::vector<RTreeEntry> entries;
  std::shared_ptr<RTreeNode> parent;

  RTreeNode(NodeType type) : type{type}, parent{nullptr} {
    entries.reserve(MAX_RTREE_ENTRIES);
  }

  bool isLeaf() const {
    return type == NodeType::LEAF;
  }
  bool isFull() const {
    return entries.size() >= MAX_RTREE_ENTRIES;
  }

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
  void insert(const BoundingBox& bbox, Data data);

  // Query elements within a bounding box
  void query(const BoundingBox& bbox,
             std::vector<Data>& results) const;

  // Query elements within radius of a point
  void queryRadius(const Point2D& center, double radius,
                   std::vector<Data>& results) const;

  // Clear all entries
  void clear();

  // Get statistics
  size_t size() const {
    return elementCount_;
  }
  size_t height() const;

 private:
  std::shared_ptr<RTreeNode> root_;
  size_t elementCount_;

  // Helper methods
  std::shared_ptr<RTreeNode> chooseLeaf(const BoundingBox& bbox);
  void splitNode(std::shared_ptr<RTreeNode>& node, RTreeEntry& newEntry);
  void adjustTree(std::shared_ptr<RTreeNode>& leaf);
  void queryNode(const std::shared_ptr<const RTreeNode>& node,
                 const BoundingBox& bbox,
                 std::vector<Data>& results) const;
  double computeEnlargement(const BoundingBox& existing,
                            const BoundingBox& addition) const;
};

}  // namespace hdmap
