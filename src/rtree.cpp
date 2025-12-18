#include "rtree.hpp"
#include <algorithm>
#include <limits>
#include <cmath>

namespace hdmap {

BoundingBox RTreeNode::getBoundingBox() const {
    if (entries_.empty()) {
        return BoundingBox();
    }
    
    BoundingBox result = entries_[0].bbox_;
    for (size_t i = 1; i < entries_.size(); ++i) {
        const auto& bbox = entries_[i].bbox_;
        result.min_.x_ = std::min(result.min_.x_, bbox.min_.x_);
        result.min_.y_ = std::min(result.min_.y_, bbox.min_.y_);
        result.max_.x_ = std::max(result.max_.x_, bbox.max_.x_);
        result.max_.y_ = std::max(result.max_.y_, bbox.max_.y_);
    }
    
    return result;
}

// use {} to differentiate between initialization and function call!
RTree::RTree() : root_{new RTreeNode{NodeType::LEAF}}, elementCount_{0} {
}

RTree::~RTree() {
    clear();
}

void RTree::insert(const BoundingBox& bbox, std::shared_ptr<Object> data) {
    RTreeEntry entry(bbox, data);
    
    if (root_->entries_.empty()) {
        root_->entries_.push_back(entry);
        elementCount_++;
        return;
    }
    
    auto leaf{chooseLeaf(bbox)};
    
    if (!leaf->isFull()) {
        leaf->entries_.push_back(entry);
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
        double minEnlargement = std::numeric_limits<double>::max();
        
        for (size_t i = 0; i < current->entries_.size(); ++i) {
            double enlargement = computeEnlargement(current->entries_[i].bbox_, bbox);
            if (enlargement < minEnlargement) {
                minEnlargement = enlargement;
                bestIdx = i;
            }
        }
        
        current = std::static_pointer_cast<RTreeNode>(current->entries_[bestIdx].data_);
    }
    
    return current;
}

double RTree::computeEnlargement(const BoundingBox& existing, const BoundingBox& addition) const {
    BoundingBox combined;
    combined.min_.x_ = std::min(existing.min_.x_, addition.min_.x_);
    combined.min_.y_ = std::min(existing.min_.y_, addition.min_.y_);
    combined.max_.x_ = std::max(existing.max_.x_, addition.max_.x_);
    combined.max_.y_ = std::max(existing.max_.y_, addition.max_.y_);
    
    return combined.area() - existing.area();
}

void RTree::splitNode(std::shared_ptr<RTreeNode>& node, RTreeEntry& newEntry) {
    // Simple linear split algorithm
    std::vector<RTreeEntry> allEntries = node->entries_;
    allEntries.push_back(newEntry);
    
    // Find seeds (entries that are farthest apart)
    size_t seed1 = 0, seed2 = 1;
    double maxDistance = 0.0;
    
    for (size_t i = 0; i < allEntries.size(); ++i) {
        for (size_t j = i + 1; j < allEntries.size(); ++j) {
            Point2D center1 = allEntries[i].bbox_.center();
            Point2D center2 = allEntries[j].bbox_.center();
            double dist = center1.distanceTo(center2);
            if (dist > maxDistance) {
                maxDistance = dist;
                seed1 = i;
                seed2 = j;
            }
        }
    }
    
    // Create new node
    auto newNode{std::make_shared<RTreeNode>(node->type_)};
    newNode->parent_ = node->parent_;
    
    // Distribute entries
    node->entries_.clear();
    node->entries_.push_back(allEntries[seed1]);
    newNode->entries_.push_back(allEntries[seed2]);
    
    for (size_t i = 0; i < allEntries.size(); ++i) {
        if (i == seed1 || i == seed2) continue;
        
        BoundingBox bbox1 = node->getBoundingBox();
        BoundingBox bbox2 = newNode->getBoundingBox();
        
        double enlarge1 = computeEnlargement(bbox1, allEntries[i].bbox_);
        double enlarge2 = computeEnlargement(bbox2, allEntries[i].bbox_);
        
        if (enlarge1 < enlarge2) {
            node->entries_.push_back(allEntries[i]);
        } else {
            newNode->entries_.push_back(allEntries[i]);
        }
    }
    
    // Handle root split
    if (node == root_) {
        auto newRoot{std::make_shared<RTreeNode>(NodeType::INTERNAL)};
        newRoot->entries_.push_back(RTreeEntry(node->getBoundingBox(), node));
        newRoot->entries_.push_back(RTreeEntry(newNode->getBoundingBox(), newNode));
        node->parent_ = newRoot;
        newNode->parent_ = newRoot;
        root_ = newRoot;
    } else {
        // Insert new node into parent
        RTreeEntry parentEntry(newNode->getBoundingBox(), newNode);
        if (!node->parent_->isFull()) {
            node->parent_->entries_.push_back(parentEntry);
        } else {
            splitNode(node->parent_, parentEntry);
        }
    }
    
    adjustTree(node);
}

void RTree::adjustTree(std::shared_ptr<RTreeNode>& leaf) {
    auto current = leaf;
    
    while (current != root_) {
        auto parent{current->parent_};
        
        // Update parent's bounding box for this child
        for (auto& entry : parent->entries_) {
            if (entry.data_ == current) {
                entry.bbox_ = current->getBoundingBox();
                break;
            }
        }
        
        current = parent;
    }
}

void RTree::query(const BoundingBox& bbox, std::vector<std::shared_ptr<Object>>& results) const {
    if (root_) {
        queryNode(root_, bbox, results);
    }
}

void RTree::queryNode(const std::shared_ptr<const RTreeNode>& node, const BoundingBox& bbox, std::vector<std::shared_ptr<Object>>& results) const {
    for (const auto& entry : node->entries_) {
        if (!entry.bbox_.intersects(bbox)) {
            continue;
        }
        
        if (node->isLeaf()) {
            results.push_back(entry.data_);
        } else {
            queryNode(std::static_pointer_cast<RTreeNode>(entry.data_), bbox, results);
        }
    }
}

void RTree::queryRadius(const Point2D& center, double radius, std::vector<std::shared_ptr<Object>>& results) const {
    BoundingBox bbox(
        Point2D(center.x_ - radius, center.y_ - radius),
        Point2D(center.x_ + radius, center.y_ + radius)
    );
    query(bbox, results);
}

void RTree::clear() {
    if (root_ && !root_->isLeaf()) {
    }
    if (root_) {
        root_->entries_.clear();
    }
    elementCount_ = 0;
}

size_t RTree::height() const {
    if (!root_) return 0;
    
    size_t h = 1;
    auto current{root_};
    while (!current->isLeaf() && !current->entries_.empty()) {
        current = std::static_pointer_cast<RTreeNode>(current->entries_[0].data_);
        h++;
    }
    return h;
}

} // namespace hdmap
