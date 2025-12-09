#include "rtree.hpp"
#include <algorithm>
#include <limits>
#include <cmath>

namespace hdmap {

BoundingBox RTreeNode::getBoundingBox() const {
    if (entries.empty()) {
        return BoundingBox();
    }
    
    BoundingBox result = entries[0].bbox;
    for (size_t i = 1; i < entries.size(); ++i) {
        const auto& bbox = entries[i].bbox;
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
    delete root_;
}

void RTree::insert(const BoundingBox& bbox, void* data) {
    RTreeEntry entry(bbox, data);
    
    if (root_->entries.empty()) {
        root_->entries.push_back(entry);
        elementCount_++;
        return;
    }
    
    RTreeNode* leaf = chooseLeaf(bbox);
    
    if (!leaf->isFull()) {
        leaf->entries.push_back(entry);
        adjustTree(leaf);
    } else {
        splitNode(leaf, entry);
    }
    
    elementCount_++;
}

RTreeNode* RTree::chooseLeaf(const BoundingBox& bbox) {
    RTreeNode* current = root_;
    
    while (!current->isLeaf()) {
        // Find entry with minimum enlargement
        size_t bestIdx = 0;
        double minEnlargement = std::numeric_limits<double>::max();
        
        for (size_t i = 0; i < current->entries.size(); ++i) {
            double enlargement = computeEnlargement(current->entries[i].bbox, bbox);
            if (enlargement < minEnlargement) {
                minEnlargement = enlargement;
                bestIdx = i;
            }
        }
        
        current = static_cast<RTreeNode*>(current->entries[bestIdx].data);
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

void RTree::splitNode(RTreeNode* node, RTreeEntry& newEntry) {
    // Simple linear split algorithm
    std::vector<RTreeEntry> allEntries = node->entries;
    allEntries.push_back(newEntry);
    
    // Find seeds (entries that are farthest apart)
    size_t seed1 = 0, seed2 = 1;
    double maxDistance = 0.0;
    
    for (size_t i = 0; i < allEntries.size(); ++i) {
        for (size_t j = i + 1; j < allEntries.size(); ++j) {
            Point2D center1 = allEntries[i].bbox.center();
            Point2D center2 = allEntries[j].bbox.center();
            double dist = center1.distanceTo(center2);
            if (dist > maxDistance) {
                maxDistance = dist;
                seed1 = i;
                seed2 = j;
            }
        }
    }
    
    // Create new node
    RTreeNode* newNode = new RTreeNode(node->type);
    newNode->parent = node->parent;
    
    // Distribute entries
    node->entries.clear();
    node->entries.push_back(allEntries[seed1]);
    newNode->entries.push_back(allEntries[seed2]);
    
    for (size_t i = 0; i < allEntries.size(); ++i) {
        if (i == seed1 || i == seed2) continue;
        
        BoundingBox bbox1 = node->getBoundingBox();
        BoundingBox bbox2 = newNode->getBoundingBox();
        
        double enlarge1 = computeEnlargement(bbox1, allEntries[i].bbox);
        double enlarge2 = computeEnlargement(bbox2, allEntries[i].bbox);
        
        if (enlarge1 < enlarge2) {
            node->entries.push_back(allEntries[i]);
        } else {
            newNode->entries.push_back(allEntries[i]);
        }
    }
    
    // Handle root split
    if (node == root_) {
        RTreeNode* newRoot = new RTreeNode(NodeType::INTERNAL);
        newRoot->entries.push_back(RTreeEntry(node->getBoundingBox(), node));
        newRoot->entries.push_back(RTreeEntry(newNode->getBoundingBox(), newNode));
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

void RTree::adjustTree(RTreeNode* leaf) {
    RTreeNode* current = leaf;
    
    while (current != root_) {
        RTreeNode* parent = current->parent;
        
        // Update parent's bounding box for this child
        for (auto& entry : parent->entries) {
            if (entry.data == current) {
                entry.bbox = current->getBoundingBox();
                break;
            }
        }
        
        current = parent;
    }
}

void RTree::query(const BoundingBox& bbox, std::vector<void*>& results) const {
    if (root_) {
        queryNode(root_, bbox, results);
    }
}

void RTree::queryNode(const RTreeNode* node, const BoundingBox& bbox, std::vector<void*>& results) const {
    for (const auto& entry : node->entries) {
        if (!entry.bbox.intersects(bbox)) {
            continue;
        }
        
        if (node->isLeaf()) {
            results.push_back(entry.data);
        } else {
            queryNode(static_cast<RTreeNode*>(entry.data), bbox, results);
        }
    }
}

void RTree::queryRadius(const Point2D& center, double radius, std::vector<void*>& results) const {
    BoundingBox bbox(
        Point2D(center.x_ - radius, center.y_ - radius),
        Point2D(center.x_ + radius, center.y_ + radius)
    );
    query(bbox, results);
}

void RTree::clear() {
    if (root_ && !root_->isLeaf()) {
        deleteTree(root_);
    }
    if (root_) {
        root_->entries.clear();
    }
    elementCount_ = 0;
}

void RTree::deleteTree(RTreeNode* node) {
    if (!node->isLeaf()) {
        for (auto& entry : node->entries) {
            deleteTree(static_cast<RTreeNode*>(entry.data));
        }
    }
    if (node != root_) {
        delete node;
    }
}

size_t RTree::height() const {
    if (!root_) return 0;
    
    size_t h = 1;
    RTreeNode* current = root_;
    while (!current->isLeaf() && !current->entries.empty()) {
        current = static_cast<RTreeNode*>(current->entries[0].data);
        h++;
    }
    return h;
}

} // namespace hdmap
