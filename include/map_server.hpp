#pragma once

#include "types.hpp"
#include "rtree.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>

namespace hdmap {

// yiliang 
// shall I make a singleton? 
// Memory constraints for embedded systems
struct MemoryConstraints {
    size_t maxTotalMemory;      // bytes
    size_t maxLanes;
    size_t maxTrafficLights;
    size_t maxTrafficSigns;
    
    static MemoryConstraints defaultConstraints() {
        return {
            64 * 1024 * 1024,  // 64 MB total
            10000,              // Max 10k lanes
            5000,               // Max 5k traffic lights
            5000                // Max 5k traffic signs
        };
    }
    
    static MemoryConstraints raspberryPi() {
        return {
            128 * 1024 * 1024,  // 128 MB
            20000,
            10000,
            10000
        };
    }
};

// Main HD Map Server API
class MapServer {
public:
    explicit MapServer(const MemoryConstraints& constraints = MemoryConstraints::defaultConstraints());
    ~MapServer();
    
    // Disable copy, enable move
    MapServer(const MapServer&) = delete;
    MapServer& operator=(const MapServer&) = delete;
    MapServer(MapServer&&) = default;
    MapServer& operator=(MapServer&&) = default;
    
    // Load map from file
    bool loadFromFile(const std::string& filepath);
    
    // Query API - main interface for autonomous driving
    QueryResult queryRegion(const BoundingBox& region) const;
    QueryResult queryRadius(const Point2D& center, double radius) const;
    
    std::optional<std::shared_ptr<Lane>> getLaneById(uint64_t laneId) const;
    std::optional<std::shared_ptr<TrafficLight>> getTrafficLightById(uint64_t id) const;
    std::optional<std::shared_ptr<TrafficSign>> getTrafficSignById(uint64_t id) const;
    
    // Get lanes within distance of a point
    std::vector<std::shared_ptr<Lane>> getNearbyLanes(const Point2D& position, double maxDistance) const;
    
    // Get closest lane to a position
    std::optional<std::shared_ptr<Lane>> getClosestLane(const Point2D& position) const;
    
    // Get traffic lights controlling a specific lane
    std::vector<std::shared_ptr<TrafficLight>> getTrafficLightsForLane(uint64_t laneId) const;
    
    // Get traffic signs affecting a specific lane
    std::vector<std::shared_ptr<TrafficSign>> getTrafficSignsForLane(uint64_t laneId) const;
    
    // Statistics and memory usage
    size_t getLaneCount() const { return lanes_.size(); }
    size_t getTrafficLightCount() const { return trafficLights_.size(); }
    size_t getTrafficSignCount() const { return trafficSigns_.size(); }
    size_t getMemoryUsage() const;
    std::unordered_map<uint64_t, std::shared_ptr<Lane>>& getLanes() {return lanes_;}
    std::unordered_map<uint64_t, std::shared_ptr<TrafficLight>>& getTrafficLights() {return trafficLights_;}
    std::unordered_map<uint64_t, std::shared_ptr<TrafficSign>>& getTrafficSigns(){return  trafficSigns_;}
    
    // Clear all map data
    void clear();
    
private:
    MemoryConstraints constraints_;
    
    // Map data storage
    std::unordered_map<uint64_t, std::shared_ptr<Lane>> lanes_;
    std::unordered_map<uint64_t, std::shared_ptr<TrafficLight>> trafficLights_;
    std::unordered_map<uint64_t, std::shared_ptr<TrafficSign>> trafficSigns_;
    
    // Spatial indices for fast queries
    RTree laneIndex_;
    RTree trafficLightIndex_;
    RTree trafficSignIndex_;
    
    // Helper methods
    bool checkMemoryConstraints() const;
    void buildSpatialIndices();
};

} // namespace hdmap
