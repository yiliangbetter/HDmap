#include "map_server.hpp"
#include <iostream>
#include <iomanip>

// yiliang 
// use smart pointer instead of raw pointers (move huge memory consumption from stack to heap, 
// would that result in the change of the size of the iamge)
// try to find the hotspot and try to optimize this program in terms of 
// runtime and memory usage!
// multi-threading
// modern cpp refurbishment
// instead of using Cmake. try to use bazel to do the same thing! 
// avoid using void* as a way to express generic


using namespace hdmap;

void printQueryResult(const QueryResult& result) {
    std::cout << "Query Results:\n";
    std::cout << "  Lanes: " << result.lanes_.size() << "\n";
    std::cout << "  Traffic Lights: " << result.trafficLights_.size() << "\n";
    std::cout << "  Traffic Signs: " << result.trafficSigns_.size() << "\n";
    
    if (!result.lanes_.empty()) {
        std::cout << "\n  Lane Details:\n";
        for (auto lane : result.lanes_) {
            std::cout << "    ID: " << lane->id_ 
                     << ", Points: " << lane->centerline_.size()
                     << ", Speed Limit: " << (lane->speedLimit_ * 3.6) << " km/h\n";
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
            std::cout << "  Current (soft): " 
                      << (limit.rlim_cur / 1024 / 1024) << " MB\n";
        }
        
        if (limit.rlim_max == RLIM_INFINITY) {
            std::cout << "  Maximum (hard): unlimited\n";
        } else {
            std::cout << "  Maximum (hard): " 
                      << (limit.rlim_max / 1024 / 1024) << " MB\n";
        }
    }
}

int main(int argc, char** argv) {
    printStackLimit();
    std::cout << "=== HD Map Server Demo ===\n\n";
    
    // Create map server with default constraints
    MapServer mapServer(MemoryConstraints::defaultConstraints());
    
    // Load map data
    std::string mapFile = "data/sample_map.osm";
    if (argc > 1) {
        mapFile = argv[1];
    }
    
    std::cout << "Loading map from: " << mapFile << "\n";
    if (!mapServer.loadFromFile(mapFile)) {
        std::cerr << "Failed to load map file!\n";
        return 1;
    }
    
    std::cout << "Map loaded successfully!\n\n";
    
    // Print statistics
    std::cout << "Map Statistics:\n";
    std::cout << "  Lanes: " << mapServer.getLaneCount() << "\n";
    std::cout << "  Traffic Lights: " << mapServer.getTrafficLightCount() << "\n";
    std::cout << "  Traffic Signs: " << mapServer.getTrafficSignCount() << "\n";
    std::cout << "  Memory Usage: " << (mapServer.getMemoryUsage() / 1024.0 / 1024.0) 
              << " MB\n\n";
    
    // Example queries
    std::cout << "=== Example Queries ===\n\n";
    
    // Query 1: Region query
    std::cout << "1. Querying region (0, 0) to (100, 100):\n";
    BoundingBox region(Point2D(0, 0), Point2D(100, 100));
    QueryResult result1 = mapServer.queryRegion(region);
    printQueryResult(result1);
    
    // Query 2: Radius query
    std::cout << "\n2. Querying 50m radius around (50, 50):\n";
    Point2D center(50, 50);
    QueryResult result2 = mapServer.queryRadius(center, 50.0);
    printQueryResult(result2);
    
    // Query 3: Closest lane
    std::cout << "\n3. Finding closest lane to (25, 25):\n";
    Point2D position(25, 25);
    auto closestLane = mapServer.getClosestLane(position);
    if (closestLane.has_value()) {
        auto lane = closestLane.value();
        std::cout << "  Found lane ID: " << lane->id_ << "\n";
        std::cout << "  Points in centerline: " << lane->centerline_.size() << "\n";
    } else {
        std::cout << "  No lane found nearby\n";
    }
    
    std::cout << "\n=== Demo Complete ===\n";
    
    return 0;
}
