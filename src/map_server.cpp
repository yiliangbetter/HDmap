#include "map_server.hpp"
#include "lanelet2_parser.hpp"
#include <algorithm>
#include <limits>

// yiliang
// make MapServer a singleton
// make changes to reflect univeral references
// review solid principles and try to find out if this piece of code actually violates
// that principle?
// perfect forwarding
// do profiling, learn how to set up profiling tool
// set up google benchmark to do more meaning runtime test

namespace hdmap {

MapServer::MapServer(const MemoryConstraints& constraints)
    : constraints_(constraints) {
}

MapServer::~MapServer() {
    clear();
}

bool MapServer::loadFromFile(const std::string& filepath) {
    clear();
    
    Lanelet2Parser parser;
    if (!parser.parse(filepath, *this)) {
        return false;
    }
    
    if (!checkMemoryConstraints()) {
        clear();
        return false;
    }
    
    buildSpatialIndices();
    return true;
}

void MapServer::buildSpatialIndices() {
    // Build lane index
    laneIndex_.clear();
    for (auto& [id, lane] : lanes_) {
        lane->computeBoundingBox();
        // looks pretty fishy here, I got to say! 
        laneIndex_.insert(lane->bbox_, lane.get());
    }
    
    // Build traffic light index
    trafficLightIndex_.clear();
    for (auto& [id, light] : trafficLights_) {
        BoundingBox bbox(light->position_, light->position_);
        trafficLightIndex_.insert(bbox, light.get());
    }
    
    // Build traffic sign index
    trafficSignIndex_.clear();
    for (auto& [id, sign] : trafficSigns_) {
        BoundingBox bbox(sign->position_, sign->position_);
        trafficSignIndex_.insert(bbox, sign.get());
    }
}

QueryResult MapServer::queryRegion(const BoundingBox& region) const {
    QueryResult result;
    
    // Query lanes
    std::vector<void*> laneResults;
    laneIndex_.query(region, laneResults);
    for (void* ptr : laneResults) {
        result.lanes_.push_back(std::shared_ptr<Lane>{static_cast<Lane*>(ptr)});
    }
    
    // Query traffic lights
    std::vector<void*> lightResults;
    trafficLightIndex_.query(region, lightResults);
    for (void* ptr : lightResults) {
        result.trafficLights_.push_back(std::shared_ptr<TrafficLight>{static_cast<TrafficLight*>(ptr)});
    }
    
    // Query traffic signs
    std::vector<void*> signResults;
    trafficSignIndex_.query(region, signResults);
    for (void* ptr : signResults) {
        result.trafficSigns_.push_back(std::shared_ptr<TrafficSign>{static_cast<TrafficSign*>(ptr)});
    }
    
    // return value optimization
    return result;
}

QueryResult MapServer::queryRadius(const Point2D& center, double radius) const {
    QueryResult result;
    
    // Query lanes
    std::vector<void*> laneResults;
    laneIndex_.queryRadius(center, radius, laneResults);
    for (void* ptr : laneResults) {
        auto lane{std::shared_ptr<Lane>(static_cast<Lane*>(ptr))};
        // Double-check distance for accuracy
        bool withinRadius = false;
        for (const auto& point : lane->centerline_) {
            if (center.distanceTo(point) <= radius) {
                withinRadius = true;
                break;
            }
        }
        if (withinRadius) {
            result.lanes_.push_back(lane);
        }
    }
    
    // Query traffic lights
    std::vector<void*> lightResults;
    // here needs to be changed as well
    trafficLightIndex_.queryRadius(center, radius, lightResults);
    for (void* ptr : lightResults) {
        auto light{std::shared_ptr<TrafficLight>(static_cast<TrafficLight*>(ptr))};
        if (center.distanceTo(light->position_) <= radius) {
            result.trafficLights_.push_back(light);
        }
    }
    
    // Query traffic signs
    std::vector<void*> signResults;
    trafficSignIndex_.queryRadius(center, radius, signResults);
    for (void* ptr : signResults) {
        auto sign{std::shared_ptr<TrafficSign>(static_cast<TrafficSign*>(ptr))};
        
        if (center.distanceTo(sign->position_) <= radius) {
            result.trafficSigns_.push_back(sign);
        }
    }
    
    return result;
}

std::optional<std::shared_ptr<Lane>> MapServer::getLaneById(uint64_t laneId) const {
    auto it = lanes_.find(laneId);
    if (it != lanes_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::shared_ptr<TrafficLight>> MapServer::getTrafficLightById(uint64_t id) const {
    auto it = trafficLights_.find(id);
    if (it != trafficLights_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::shared_ptr<TrafficSign>> MapServer::getTrafficSignById(uint64_t id) const {
    auto it = trafficSigns_.find(id);
    if (it != trafficSigns_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::shared_ptr<Lane>> MapServer::getNearbyLanes(const Point2D& position, double maxDistance) const {
    QueryResult result = queryRadius(position, maxDistance);
    return result.lanes_;
}

std::optional<std::shared_ptr<Lane>> MapServer::getClosestLane(const Point2D& position) const {
    // Start with a reasonable search radius
    double searchRadius = 50.0;  // meters
    auto candidates{getNearbyLanes(position, searchRadius)};
    
    if (candidates.empty()) {
        // Try larger radius
        searchRadius = 200.0;
        candidates = getNearbyLanes(position, searchRadius);
        if (candidates.empty()) {
            return std::nullopt;
        }
    }
    
    // Find closest lane by checking distance to centerline points
    std::shared_ptr<Lane> closestLane = nullptr;
    double minDistance = std::numeric_limits<double>::max();
    
    for (auto lane : candidates) {
        for (const auto& point : lane->centerline_) {
            double dist = position.distanceTo(point);
            if (dist < minDistance) {
                minDistance = dist;
                closestLane = lane;
            }
        }
    }
    
    return closestLane;
}

std::vector<std::shared_ptr<TrafficLight>> MapServer::getTrafficLightsForLane(uint64_t laneId) const {
    std::vector<std::shared_ptr<TrafficLight>> result;
    
    for (const auto& [id, light] : trafficLights_) {
        auto it = std::find(light->controlledLaneIds_.begin(), 
                           light->controlledLaneIds_.end(), 
                           laneId);
        if (it != light->controlledLaneIds_.end()) {
            result.push_back(light);
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<TrafficSign>> MapServer::getTrafficSignsForLane(uint64_t laneId) const {
    std::vector<std::shared_ptr<TrafficSign>> result;
    
    for (const auto& [id, sign] : trafficSigns_) {
        auto it = std::find(sign->affectedLaneIds_.begin(), 
                           sign->affectedLaneIds_.end(), 
                           laneId);
        if (it != sign->affectedLaneIds_.end()) {
            result.push_back(sign);
        }
    }
    
    return result;
}

size_t MapServer::getMemoryUsage() const {
    size_t total = 0;
    
    // Estimate lane memory
    for (const auto& [id, lane] : lanes_) {
        total += sizeof(Lane);
        total += lane->centerline_.size() * sizeof(Point2D);
        total += lane->leftBoundary_.size() * sizeof(Point2D);
        total += lane->rightBoundary_.size() * sizeof(Point2D);
        total += lane->predecessorIds_.size() * sizeof(uint64_t);
        total += lane->successorIds_.size() * sizeof(uint64_t);
        total += lane->adjacentLeftIds_.size() * sizeof(uint64_t);
        total += lane->adjacentRightIds_.size() * sizeof(uint64_t);
    }
    
    // Traffic lights
    total += trafficLights_.size() * sizeof(TrafficLight);
    for (const auto& [id, light] : trafficLights_) {
        total += light->controlledLaneIds_.size() * sizeof(uint64_t);
    }
    
    // Traffic signs
    total += trafficSigns_.size() * sizeof(TrafficSign);
    for (const auto& [id, sign] : trafficSigns_) {
        total += sign->value_.capacity();
        total += sign->affectedLaneIds_.size() * sizeof(uint64_t);
    }
    
    // R-tree overhead (rough estimate)
    total += (laneIndex_.size() + trafficLightIndex_.size() + trafficSignIndex_.size()) * 64;
    
    return total;
}

bool MapServer::checkMemoryConstraints() const {
    if (lanes_.size() > constraints_.maxLanes) return false;
    if (trafficLights_.size() > constraints_.maxTrafficLights) return false;
    if (trafficSigns_.size() > constraints_.maxTrafficSigns) return false;
    
    size_t memUsage = getMemoryUsage();
    return memUsage <= constraints_.maxTotalMemory;
}

void MapServer::clear() {
    lanes_.clear();
    trafficLights_.clear();
    trafficSigns_.clear();
    laneIndex_.clear();
    trafficLightIndex_.clear();
    trafficSignIndex_.clear();
}

} // namespace hdmap
