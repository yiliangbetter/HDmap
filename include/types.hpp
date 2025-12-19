#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace hdmap {

// Basic geometric types
struct Point2D {
  double x;
  double y;

  Point2D() : x(0.0), y(0.0) {
  }
  Point2D(double x, double y) : x(x), y(y) {
  }

  double distanceTo(const Point2D& other) const;
};

struct BoundingBox {
  Point2D min;
  Point2D max;

  BoundingBox() = default;
  BoundingBox(const Point2D& min, const Point2D& max) : min(min), max(max) {
  }

  bool contains(const Point2D& point) const;
  bool intersects(const BoundingBox& other) const;
  double area() const;
  Point2D center() const;
};

// Map element types
enum class LaneType : uint8_t {
  DRIVING,
  SIDEWALK,
  BIKE_LANE,
  PARKING,
  SHOULDER,
  RESTRICTED
};

enum class TrafficLightState : uint8_t {
  RED,
  YELLOW,
  GREEN,
  RED_YELLOW,
  UNKNOWN
};

enum class TrafficSignType : uint8_t {
  STOP,
  YIELD,
  SPEED_LIMIT,
  NO_ENTRY,
  ONE_WAY,
  PARKING,
  PEDESTRIAN_CROSSING,
  SCHOOL_ZONE,
  OTHER
};

// Core map elements
struct Object {
  uint64_t id;
  Object() : id{0} {
  }
  explicit Object(uint64_t id) : id{id} {
  }
};

struct Lane : public Object {
  LaneType type;
  std::vector<Point2D> centerline;
  std::vector<Point2D> leftBoundary;
  std::vector<Point2D> rightBoundary;
  std::vector<uint64_t> predecessorIds;
  std::vector<uint64_t> successorIds;
  std::vector<uint64_t> adjacentLeftIds;
  std::vector<uint64_t> adjacentRightIds;
  double speedLimit;  // m/s
  BoundingBox bbox;

  Lane() : type{LaneType::DRIVING}, speedLimit{0.0} {
  }

  void computeBoundingBox();
};

struct TrafficLight : public Object {
  Point2D position;
  TrafficLightState state;
  std::vector<uint64_t> controlledLaneIds;
  double height;  // meters above ground

  TrafficLight() : state{TrafficLightState::UNKNOWN}, height{0.0} {
  }
};

struct TrafficSign : public Object {
  Point2D position;
  TrafficSignType type;
  std::string value;  // e.g., "50" for speed limit
  std::vector<uint64_t> affectedLaneIds;
  double height;  // meters above ground

  TrafficSign() : type{TrafficSignType::OTHER}, height{0.0} {
  }
};

// Map query result structures
struct QueryResult {
  std::vector<std::shared_ptr<Lane>> lanes;
  std::vector<std::shared_ptr<TrafficLight>> trafficLights;
  std::vector<std::shared_ptr<TrafficSign>> trafficSigns;

  void clear() {
    lanes.clear();
    trafficLights.clear();
    trafficSigns.clear();
  }

  size_t totalCount() const {
    return lanes.size() + trafficLights.size() + trafficSigns.size();
  }
};

}  // namespace hdmap
