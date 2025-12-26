#include <iomanip>
#include <iostream>

#include "map_server.hpp"

using namespace hdmap;

void printQueryResult(const QueryResult& result) {
  std::cout << "Query Results:\n";
  std::cout << "  Lanes: " << result.lanes.size() << "\n";
  std::cout << "  Traffic Lights: " << result.trafficLights.size() << "\n";
  std::cout << "  Traffic Signs: " << result.trafficSigns.size() << "\n";

  if (!result.lanes.empty()) {
    std::cout << "\n  Lane Details:\n";
    for (const auto& lane : result.lanes) {
      std::cout << "    ID: " << lane->id
                << ", Points: " << lane->centerline.size()
                << ", Speed Limit: " << (lane->speedLimit * 3.6) << " km/h\n";
    }
  }
}

void printStackLimit() {
  struct rlimit limit;

  if (getrlimit(RLIMIT_STACK, &limit) == 0) {
    std::cout << "Stack size limit:\n";

    if (limit.rlim_cur == RLIM_INFINITY) {
      std::cout << "  Current (soft): unlimited\n";
    } else {
      std::cout << "  Current (soft): " << (limit.rlim_cur / 1024 / 1024)
                << " MB\n";
    }

    if (limit.rlim_max == RLIM_INFINITY) {
      std::cout << "  Maximum (hard): unlimited\n";
    } else {
      std::cout << "  Maximum (hard): " << (limit.rlim_max / 1024 / 1024)
                << " MB\n";
    }
  }
}

int main(int argc, char** argv) {
  printStackLimit();
  std::cout << "=== HD Map Server Demo ===\n\n";

  // Create map server with default constraints
  auto mapServer{
      MapServer::getInstance(MemoryConstraints::defaultConstraints())};

  // Load map data
  std::string mapFile = "data/sample_map.osm";
  if (argc > 1) {
    mapFile = argv[1];
  }

  std::cout << "Loading map from: " << mapFile << "\n";
  if (!mapServer->loadFromFile(std::move(mapFile))) {
    std::cerr << "Failed to load map file!\n";
    return 1;
  }

  std::cout << "Map loaded successfully!\n\n";

  // Print statistics
  std::cout << "Map Statistics:\n";
  std::cout << "  Lanes: " << mapServer->getLaneCount() << "\n";
  std::cout << "  Traffic Lights: " << mapServer->getTrafficLightCount()
            << "\n";
  std::cout << "  Traffic Signs: " << mapServer->getTrafficSignCount() << "\n";
  std::cout << "  Memory Usage: "
            << (mapServer->getMemoryUsage() / 1024.0 / 1024.0) << " MB\n\n";

  // Example queries
  std::cout << "=== Example Queries ===\n\n";

  // Query 1: Region query
  std::cout << "1. Querying region (0, 0) to (100, 100):\n";
  const BoundingBox region{Point2D(0, 0), Point2D(100, 100)};
  const QueryResult result1{mapServer->queryRegion(region)};
  printQueryResult(result1);

  // Query 2: Radius query
  std::cout << "\n2. Querying 50m radius around (50, 50):\n";
  const Point2D center{50, 50};
  const QueryResult result2{mapServer->queryRadius(center, 50.0)};
  printQueryResult(result2);

  // Query 3: Closest lane
  std::cout << "\n3. Finding closest lane to (25, 25):\n";
  const Point2D position{25, 25};
  auto closestLane = mapServer->getClosestLane(position);
  if (closestLane.has_value()) {
    const auto& lane{closestLane.value()};
    std::cout << "  Found lane ID: " << lane->id << "\n";
    std::cout << "  Points in centerline: " << lane->centerline.size() << "\n";
  } else {
    std::cout << "  No lane found nearby\n";
  }

  std::cout << "\n=== Demo Complete ===\n";

  return 0;
}
