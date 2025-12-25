#include "rtree.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace hdmap {

BoundingBox RTreeNode::getBoundingBox() const {
  if (entries.empty()) {
    return BoundingBox{};
  }

  BoundingBox result = entries[0].bbox;
  for (size_t i = 1; i < entries.size(); ++i) {
    const auto& bbox = entries[i].bbox;
    result.min.x = std::min(result.min.x, bbox.min.x);
    result.min.y = std::min(result.min.y, bbox.min.y);
    result.max.x = std::max(result.max.x, bbox.max.x);
    result.max.y = std::max(result.max.y, bbox.max.y);
  }

  return result;
}

// use {} to differentiate between initialization and function call!
RTree::RTree() : root_{new RTreeNode{NodeType::LEAF}}, elementCount_{0} {
}

RTree::~RTree() {
  clear();
}

void RTree::insert(const BoundingBox& bbox, Data data) {
  RTreeEntry entry(bbox, data);

  if (root_->entries.empty()) {
    root_->entries.push_back(entry);
    elementCount_++;
    return;
  }

  auto leaf{chooseLeaf(bbox)};

  if (!leaf->isFull()) {
    leaf->entries.push_back(entry);
    adjustTree(leaf);
  } else {
    splitNode(leaf, entry);
  }

  elementCount_++;
}

std::shared_ptr<RTreeNode> RTree::chooseLeaf(const BoundingBox& bbox) {
  auto current{root_};

  while (!current->isLeaf()) {
    // Find entry with minimum enlargement
    size_t bestIdx = 0;
    auto minEnlargement = std::numeric_limits<double>::max();

    for (size_t i = 0; i < current->entries.size(); ++i) {
      const auto enlargement{computeEnlargement(current->entries[i].bbox, bbox)};
      if (enlargement < minEnlargement) {
        minEnlargement = enlargement;
        bestIdx = i;
      }
    }

    current = std::get<std::shared_ptr<RTreeNode>>(current->entries[bestIdx].data);
  }

  return current;
}

double RTree::computeEnlargement(const BoundingBox& existing, const BoundingBox& addition) const {
  BoundingBox combined;
  combined.min.x = std::min(existing.min.x, addition.min.x);
  combined.min.y = std::min(existing.min.y, addition.min.y);
  combined.max.x = std::max(existing.max.x, addition.max.x);
  combined.max.y = std::max(existing.max.y, addition.max.y);

  return combined.area() - existing.area();
}

void RTree::splitNode(std::shared_ptr<RTreeNode>& node, RTreeEntry& newEntry) {
  // Simple linear split algorithm
  std::vector<RTreeEntry> allEntries = node->entries;
  allEntries.push_back(newEntry);

  // Find seeds (entries that are farthest apart)
  size_t seed1 = 0, seed2 = 1;
  double maxDistance = 0.0;

  for (size_t i = 0; i < allEntries.size(); ++i) {
    for (size_t j = i + 1; j < allEntries.size(); ++j) {
      const auto center1{allEntries[i].bbox.center()};
      const auto center2{allEntries[j].bbox.center()};
      const auto dist{center1.distanceTo(center2)};
      if (dist > maxDistance) {
        maxDistance = dist;
        seed1 = i;
        seed2 = j;
      }
    }
  }

  // Create new node
  auto newNode{std::make_shared<RTreeNode>(node->type)};
  newNode->parent = node->parent;

  // Distribute entries
  node->entries.clear();
  node->entries.push_back(allEntries[seed1]);
  newNode->entries.push_back(allEntries[seed2]);

  for (size_t i = 0; i < allEntries.size(); ++i) {
    if (i == seed1 || i == seed2) continue;

    const auto bbox1{node->getBoundingBox()};
    const auto bbox2{newNode->getBoundingBox()};

    const auto enlarge1{computeEnlargement(bbox1, allEntries[i].bbox)};
    const auto enlarge2{computeEnlargement(bbox2, allEntries[i].bbox)};

    if (enlarge1 < enlarge2) {
      node->entries.push_back(allEntries[i]);
    } else {
      newNode->entries.push_back(allEntries[i]);
    }
  }

  // Handle root split
  if (node == root_) {
    auto newRoot{std::make_shared<RTreeNode>(NodeType::INTERNAL)};
    newRoot->entries.emplace_back(node->getBoundingBox(), node);
    newRoot->entries.emplace_back(newNode->getBoundingBox(), newNode);
    node->parent = newRoot;
    newNode->parent = newRoot;
    root_ = newRoot;
  } else {
    // Insert new node into parent
    RTreeEntry parentEntry(newNode->getBoundingBox(), newNode);
    if (!node->parent->isFull()) {
      node->parent->entries.push_back(parentEntry);
    } else {
      splitNode(node->parent, parentEntry);
    }
  }

  adjustTree(node);
}

void RTree::adjustTree(std::shared_ptr<RTreeNode>& leaf) {
  auto current = leaf;

  while (current != root_) {
    auto parent{current->parent};

    // Update parent's bounding box for this child
    for (auto& entry : parent->entries) {
      if (std::get<std::shared_ptr<RTreeNode>>(entry.data) == current) {
        entry.bbox = current->getBoundingBox();
        break;
      }
    }

    current = parent;
  }
}

void RTree::query(const BoundingBox& bbox, std::vector<Data>& results) const {
  if (root_) {
    queryNode(root_, bbox, results);
  }
}

void RTree::queryNode(const std::shared_ptr<const RTreeNode>& node, const BoundingBox& bbox,
                      std::vector<Data>& results) const {
  for (const auto& entry : node->entries) {
    if (!entry.bbox.intersects(bbox)) {
      continue;
    }

    if (node->isLeaf()) {
      results.push_back(entry.data);
    } else {
      queryNode(std::get<std::shared_ptr<RTreeNode>>(entry.data), bbox, results);
    }
  }
}

void RTree::queryRadius(const Point2D& center, double radius, std::vector<Data>& results) const {
  const BoundingBox bbox{Point2D(center.x - radius, center.y - radius), Point2D(center.x + radius, center.y + radius)};
  query(bbox, results);
}

void RTree::clear() {
  if (root_ && !root_->isLeaf()) {
  }
  if (root_) {
    root_->entries.clear();
  }
  elementCount_ = 0;
}

size_t RTree::height() const {
  if (!root_) return 0;

  size_t h = 1;
  auto current{root_};
  while (!current->isLeaf() && !current->entries.empty()) {
    current = std::get<std::shared_ptr<RTreeNode>>(current->entries[0].data);
    h++;
  }
  return h;
}

}  // namespace hdmap
