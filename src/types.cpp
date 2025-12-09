#include "types.hpp"
#include <cmath>
#include <algorithm>

namespace hdmap {

double Point2D::distanceTo(const Point2D& other) const {
    const double dx = x_ - other.x_;
    const double dy = y_ - other.y_;
    return std::sqrt(dx * dx + dy * dy);
}

bool BoundingBox::contains(const Point2D& point) const {
    return point.x_ >= min_.x_ && point.x_ <= max_.x_ &&
           point.y_ >= min_.y_ && point.y_ <= max_.y_;
}

bool BoundingBox::intersects(const BoundingBox& other) const {
    return !(max_.x_ < other.min_.x_ || min_.x_ > other.max_.x_ ||
             max_.y_ < other.min_.y_ || min_.y_ > other.max_.y_);
}

double BoundingBox::area() const {
    return (max_.x_ - min_.x_) * (max_.y_ - min_.y_);
}

Point2D BoundingBox::center() const {
    return Point2D((min_.x_ + max_.x_) / 2.0, (min_.y_ + max_.y_) / 2.0);
}

void Lane::computeBoundingBox() {
    if (centerline_.empty()) {
        bbox_ = BoundingBox();
        return;
    }
    
    double minX = centerline_[0].x_;
    double maxX = centerline_[0].x_;
    double minY = centerline_[0].y_;
    double maxY = centerline_[0].y_;
    
    for (const auto& point : centerline_) {
        minX = std::min(minX, point.x_);
        maxX = std::max(maxX, point.x_);
        minY = std::min(minY, point.y_);
        maxY = std::max(maxY, point.y_);
    }
    
    for (const auto& point : leftBoundary_) {
        minX = std::min(minX, point.x_);
        maxX = std::max(maxX, point.x_);
        minY = std::min(minY, point.y_);
        maxY = std::max(maxY, point.y_);
    }
    
    for (const auto& point : rightBoundary_) {
        minX = std::min(minX, point.x_);
        maxX = std::max(maxX, point.x_);
        minY = std::min(minY, point.y_);
        maxY = std::max(maxY, point.y_);
    }
    
    bbox_ = BoundingBox(Point2D(minX, minY), Point2D(maxX, maxY));
}

} // namespace hdmap
