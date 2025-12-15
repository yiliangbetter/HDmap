#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <memory>

namespace hdmap {

// Basic geometric types
struct Point2D {
    double x_;
    double y_;
    
    Point2D() : x_(0.0), y_(0.0) {}
    Point2D(double x, double y) : x_(x), y_(y) {}
    
    double distanceTo(const Point2D& other) const;
};

struct BoundingBox {
    Point2D min_;
    Point2D max_;
    
    BoundingBox() = default;
    BoundingBox(const Point2D& min, const Point2D& max) : min_(min), max_(max) {}
    
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
struct Lane {
    uint64_t id_;
    LaneType type_;
    std::vector<Point2D> centerline_;
    std::vector<Point2D> leftBoundary_;
    std::vector<Point2D> rightBoundary_;
    std::vector<uint64_t> predecessorIds_;
    std::vector<uint64_t> successorIds_;
    std::vector<uint64_t> adjacentLeftIds_;
    std::vector<uint64_t> adjacentRightIds_;
    double speedLimit_;  // m/s
    BoundingBox bbox_;
    
    Lane() : id_{0}, type_{LaneType::DRIVING}, speedLimit_{0.0} {}
    
    void computeBoundingBox();
};

struct TrafficLight {
    uint64_t id_;
    Point2D position_;
    TrafficLightState state_;
    std::vector<uint64_t> controlledLaneIds_;
    double height_;  // meters above ground
    
    TrafficLight() : id_{0}, state_{TrafficLightState::UNKNOWN}, height_{0.0} {}
};

struct TrafficSign {
    uint64_t id_;
    Point2D position_;
    TrafficSignType type_;
    std::string value_;  // e.g., "50" for speed limit
    std::vector<uint64_t> affectedLaneIds_;
    double height_;  // meters above ground
    
    TrafficSign() : id_{0}, type_{TrafficSignType::OTHER}, height_{0.0} {}
};

// Map query result structures
struct QueryResult {
    std::vector<std::shared_ptr<Lane>> lanes_;
    std::vector<std::shared_ptr<TrafficLight>> trafficLights_;
    std::vector<std::shared_ptr<TrafficSign>> trafficSigns_;
    
    void clear() {
        lanes_.clear();
        trafficLights_.clear();
        trafficSigns_.clear();
    }
    
    size_t totalCount() const {
        return lanes_.size() + trafficLights_.size() + trafficSigns_.size();
    }
};

} // namespace hdmap
