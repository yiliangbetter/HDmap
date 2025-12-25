#include "types.hpp"

#include <algorithm>
#include <cmath>

namespace hdmap {

double Point2D::distanceTo(const Point2D& other) const {
  const double dx = x - other.x;
  const double dy = y - other.y;
  return std::sqrt(dx * dx + dy * dy);
}

bool BoundingBox::contains(const Point2D& point) const {
  return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
}

bool BoundingBox::intersects(const BoundingBox& other) const {
  return !(max.x < other.min.x || min.x > other.max.x || max.y < other.min.y || min.y > other.max.y);
}

double BoundingBox::area() const {
  return (max.x - min.x) * (max.y - min.y);
}

Point2D BoundingBox::center() const {
  return Point2D{(min.x + max.x) / 2.0, (min.y + max.y) / 2.0};
}

void Lane::computeBoundingBox() {
  if (centerline.empty()) {
    bbox = BoundingBox();
    return;
  }

  double minX = centerline[0].x;
  double maxX = centerline[0].x;
  double minY = centerline[0].y;
  double maxY = centerline[0].y;

  for (const auto& point : centerline) {
    minX = std::min(minX, point.x);
    maxX = std::max(maxX, point.x);
    minY = std::min(minY, point.y);
    maxY = std::max(maxY, point.y);
  }

  for (const auto& point : leftBoundary) {
    minX = std::min(minX, point.x);
    maxX = std::max(maxX, point.x);
    minY = std::min(minY, point.y);
    maxY = std::max(maxY, point.y);
  }

  for (const auto& point : rightBoundary) {
    minX = std::min(minX, point.x);
    maxX = std::max(maxX, point.x);
    minY = std::min(minY, point.y);
    maxY = std::max(maxY, point.y);
  }

  bbox = BoundingBox(Point2D(minX, minY), Point2D(maxX, maxY));
}

}  // namespace hdmap
