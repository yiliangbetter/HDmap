#include "map_server.hpp"

#include <algorithm>
#include <limits>

#include "lanelet2_parser.hpp"

// yiliang
// make MapServer a singleton
// cpplint
// read and extend test cases
// customize clang-tidy

namespace hdmap {

std::shared_ptr<MapServer> MapServer::instance{};
std::mutex MapServer::mutex_lock{};

MapServer::MapServer(const MemoryConstraints& constraints)
    : constraints_(constraints) {
}

std::shared_ptr<MapServer> MapServer::getInstance(
    const MemoryConstraints& constraints) {
  if (instance == nullptr) {
    const std::scoped_lock lock{mutex_lock};
    if (instance == nullptr) {
      instance = std::shared_ptr<MapServer>{new MapServer{constraints}};
    }
  }

  return instance;
}

MapServer::~MapServer() {
  clear();
}

bool MapServer::loadFromFile(std::string filepath) {
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
    laneIndex_.insert(lane->bbox, lane);
  }

  // Build traffic light index
  trafficLightIndex_.clear();
  for (auto& [id, light] : trafficLights_) {
    const BoundingBox bbox{light->position, light->position};
    trafficLightIndex_.insert(bbox, light);
  }

  // Build traffic sign index
  trafficSignIndex_.clear();
  for (auto& [id, sign] : trafficSigns_) {
    const BoundingBox bbox{sign->position, sign->position};
    trafficSignIndex_.insert(bbox, sign);
  }
}

QueryResult MapServer::queryRegion(const BoundingBox& region) const {
  QueryResult result;

  // Query lanes
  std::vector<Data> laneResults;
  laneIndex_.query(region, laneResults);
  for (const auto& object : laneResults) {
    result.lanes.push_back(std::get<std::shared_ptr<Lane>>(object));
  }

  // Query traffic lights
  std::vector<Data> lightResults;
  trafficLightIndex_.query(region, lightResults);
  for (const auto& object : lightResults) {
    result.trafficLights.push_back(
        std::get<std::shared_ptr<TrafficLight>>(object));
  }

  // Query traffic signs
  std::vector<Data> signResults;
  trafficSignIndex_.query(region, signResults);
  for (const auto& object : signResults) {
    result.trafficSigns.push_back(
        std::get<std::shared_ptr<TrafficSign>>(object));
  }

  // return value optimization
  return result;
}

QueryResult MapServer::queryRadius(const Point2D& center, double radius) const {
  QueryResult result;

  // Query lanes
  std::vector<Data> laneResults;
  laneIndex_.queryRadius(center, radius, laneResults);
  for (const auto& object : laneResults) {
    auto lane{std::get<std::shared_ptr<Lane>>(object)};
    bool withinRadius = false;
    for (const auto& point : lane->centerline) {
      if (center.distanceTo(point) <= radius) {
        withinRadius = true;
        break;
      }
    }
    if (withinRadius) {
      result.lanes.push_back(lane);
    }
  }

  // Query traffic lights
  std::vector<Data> lightResults;
  // here needs to be changed as well
  trafficLightIndex_.queryRadius(center, radius, lightResults);
  for (const auto& object : lightResults) {
    auto light{std::get<std::shared_ptr<TrafficLight>>(object)};
    if (center.distanceTo(light->position) <= radius) {
      result.trafficLights.push_back(light);
    }
  }

  // Query traffic signs
  std::vector<Data> signResults;
  trafficSignIndex_.queryRadius(center, radius, signResults);
  for (const auto& object : signResults) {
    auto sign{std::get<std::shared_ptr<TrafficSign>>(object)};
    if (center.distanceTo(sign->position) <= radius) {
      result.trafficSigns.push_back(sign);
    }
  }

  return result;
}

std::optional<std::shared_ptr<Lane>> MapServer::getLaneById(
    uint64_t laneId) const {
  auto it = lanes_.find(laneId);
  if (it != lanes_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<std::shared_ptr<TrafficLight>> MapServer::getTrafficLightById(
    uint64_t id) const {
  auto it = trafficLights_.find(id);
  if (it != trafficLights_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<std::shared_ptr<TrafficSign>> MapServer::getTrafficSignById(
    uint64_t id) const {
  auto it = trafficSigns_.find(id);
  if (it != trafficSigns_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::vector<std::shared_ptr<Lane>> MapServer::getNearbyLanes(
    const Point2D& position, double maxDistance) const {
  const QueryResult result{queryRadius(position, maxDistance)};
  return result.lanes;
}

std::optional<std::shared_ptr<Lane>> MapServer::getClosestLane(
    const Point2D& position) const {
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

  for (const auto& lane : candidates) {
    for (const auto& point : lane->centerline) {
      const auto dist{position.distanceTo(point)};
      if (dist < minDistance) {
        minDistance = dist;
        closestLane = lane;
      }
    }
  }

  return closestLane;
}

std::vector<std::shared_ptr<TrafficLight>> MapServer::getTrafficLightsForLane(
    uint64_t laneId) const {
  std::vector<std::shared_ptr<TrafficLight>> result;

  for (const auto& [id, light] : trafficLights_) {
    auto it = std::find(light->controlledLaneIds.begin(),
                        light->controlledLaneIds.end(), laneId);
    if (it != light->controlledLaneIds.end()) {
      result.push_back(light);
    }
  }

  return result;
}

std::vector<std::shared_ptr<TrafficSign>> MapServer::getTrafficSignsForLane(
    uint64_t laneId) const {
  std::vector<std::shared_ptr<TrafficSign>> result;

  for (const auto& [id, sign] : trafficSigns_) {
    auto it = std::find(sign->affectedLaneIds.begin(),
                        sign->affectedLaneIds.end(), laneId);
    if (it != sign->affectedLaneIds.end()) {
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
    total += lane->centerline.size() * sizeof(Point2D);
    total += lane->leftBoundary.size() * sizeof(Point2D);
    total += lane->rightBoundary.size() * sizeof(Point2D);
    total += lane->predecessorIds.size() * sizeof(uint64_t);
    total += lane->successorIds.size() * sizeof(uint64_t);
    total += lane->adjacentLeftIds.size() * sizeof(uint64_t);
    total += lane->adjacentRightIds.size() * sizeof(uint64_t);
  }

  // Traffic lights
  total += trafficLights_.size() * sizeof(TrafficLight);
  for (const auto& [id, light] : trafficLights_) {
    total += light->controlledLaneIds.size() * sizeof(uint64_t);
  }

  // Traffic signs
  total += trafficSigns_.size() * sizeof(TrafficSign);
  for (const auto& [id, sign] : trafficSigns_) {
    total += sign->value.capacity();
    total += sign->affectedLaneIds.size() * sizeof(uint64_t);
  }

  // R-tree overhead (rough estimate)
  total += (laneIndex_.size() + trafficLightIndex_.size() +
            trafficSignIndex_.size()) *
           64;

  return total;
}

bool MapServer::checkMemoryConstraints() const {
  if (lanes_.size() > constraints_.maxLanes) return false;
  if (trafficLights_.size() > constraints_.maxTrafficLights) return false;
  if (trafficSigns_.size() > constraints_.maxTrafficSigns) return false;

  const size_t memUsage{getMemoryUsage()};
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

}  // namespace hdmap
