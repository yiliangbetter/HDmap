# HD Map Server for Autonomous Driving

A high-performance, memory-efficient HD Map server designed for embedded autonomous vehicle systems. Mimics in-vehicle map data services with strict resource constraints.

## Features

- **Spatial Indexing**: R-tree implementation for O(log n) query performance
- **Map Format Support**: Lanelet2/OpenDRIVE compatible OSM format parser
- **Efficient Queries**: Region-based and radius-based spatial queries
- **Memory-Constrained**: Configurable limits for embedded systems (ARM/Raspberry Pi)
- **Safety-Focused**: Modern C++17 with RAII, const correctness, and bounds checking
- **Comprehensive API**: Lane queries, traffic light/sign lookups, closest lane detection

## Architecture

```
┌─────────────────────────────────────────────┐
│          Map Server API Layer               │
│  (queryRegion, queryRadius, getLaneById)    │
└─────────────────┬───────────────────────────┘
                  │
┌─────────────────┴───────────────────────────┐
│        Spatial Index (R-Tree)               │
│  Fast O(log n) queries for map elements     │
└─────────────────┬───────────────────────────┘
                  │
┌─────────────────┴───────────────────────────┐
│     Map Data Storage (Hash Maps)            │
│  Lanes, Traffic Lights, Traffic Signs       │
└─────────────────┬───────────────────────────┘
                  │
┌─────────────────┴───────────────────────────┐
│      Lanelet2 Parser (XML Reader)           │
│  Converts OSM format to internal structures  │
└─────────────────────────────────────────────┘
```

## Core Components

### Data Types (`types.hpp`)
- **Point2D**: 2D coordinate with distance calculations
- **BoundingBox**: Spatial bounds with intersection/containment checks
- **Lane**: Road lane with centerline, boundaries, connectivity
- **TrafficLight**: Traffic signal with controlled lanes
- **TrafficSign**: Regulatory signs affecting lanes

### R-Tree Index (`rtree.hpp`)
- Spatial indexing structure for fast queries
- Supports insert, region query, radius query
- Automatic node splitting and tree balancing
- Configurable node capacity (MAX_RTREE_ENTRIES)

### Map Server (`map_server.hpp`)
- Main API for autonomous driving queries
- Memory constraint enforcement
- Multi-element spatial queries
- Lane connectivity and routing support

### Parser (`lanelet2_parser.hpp`)
- Reads Lanelet2-compatible OSM XML files
- Extracts nodes, ways (lanes), and relations (traffic elements)
- Populates map server with parsed data

## Building

### Requirements
- CMake 3.14+
- C++17 compatible compiler (GCC 7+, Clang 5+)
- Google Test (auto-downloaded)

### Standard Build
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Debug Build with Sanitizers
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

### ARM Cross-Compilation
```bash
mkdir build && cd build
cmake -DBUILD_FOR_ARM=ON ..
cmake --build .
```

### With Memory Profiling
```bash
mkdir build && cd build
cmake -DENABLE_MEMORY_PROFILING=ON ..
cmake --build .
make memcheck  # Runs valgrind
```

## Running

### Demo Application
```bash
./build/hdmap_server data/sample_map.osm
```

### Unit Tests
```bash
./build/hdmap_tests
```

### With Custom Map
```bash
./build/hdmap_server /path/to/your/map.osm
```

## API Usage

### Basic Queries
```cpp
#include "map_server.hpp"

using namespace hdmap;

// Create server with memory constraints
MapServer server(MemoryConstraints::raspberryPi());

// Load map data
server.loadFromFile("map.osm");

// Query region
BoundingBox region(Point2D(0, 0), Point2D(100, 100));
QueryResult result = server.queryRegion(region);

// Query radius (e.g., 50m around vehicle)
Point2D vehiclePos(35.5, 45.2);
QueryResult nearby = server.queryRadius(vehiclePos, 50.0);

// Find closest lane
auto lane = server.getClosestLane(vehiclePos);
if (lane.has_value()) {
    std::cout << "On lane: " << lane.value()->id << "\n";
}
```

### Advanced Queries
```cpp
// Get lane by ID
auto lane = server.getLaneById(12345);

// Get traffic lights controlling a lane
auto lights = server.getTrafficLightsForLane(12345);

// Get traffic signs affecting a lane
auto signs = server.getTrafficSignsForLane(12345);

// Check memory usage
size_t mem = server.getMemoryUsage();
std::cout << "Using " << (mem / 1024.0 / 1024.0) << " MB\n";
```

## Memory Constraints

### Default Configuration
- Max Total Memory: 64 MB
- Max Lanes: 10,000
- Max Traffic Lights: 5,000
- Max Traffic Signs: 5,000

### Raspberry Pi Configuration
- Max Total Memory: 128 MB
- Max Lanes: 20,000
- Max Traffic Lights: 10,000
- Max Traffic Signs: 10,000

### Custom Configuration
```cpp
MemoryConstraints custom;
custom.maxTotalMemory = 32 * 1024 * 1024;  // 32 MB
custom.maxLanes = 5000;
custom.maxTrafficLights = 2500;
custom.maxTrafficSigns = 2500;

MapServer server(custom);
```

## Performance Characteristics

| Operation | Time Complexity | Space Complexity |
|-----------|----------------|------------------|
| Load Map | O(n log n) | O(n) |
| Region Query | O(log n + k) | O(k) |
| Radius Query | O(log n + k) | O(k) |
| Get Lane by ID | O(1) | O(1) |
| Closest Lane | O(log n + k) | O(k) |

*where n = total elements, k = results returned*

## Map Data Format

Supports Lanelet2-compatible OSM XML format:

```xml
<osm version="0.6">
  <!-- Points in space -->
  <node id="1" lat="35.681236" lon="139.767125"/>
  
  <!-- Lane centerlines -->
  <way id="100">
    <nd ref="1"/>
    <nd ref="2"/>
    <tag k="type" v="lanelet"/>
    <tag k="subtype" v="road"/>
  </way>
  
  <!-- Traffic lights -->
  <relation id="200">
    <tag k="type" v="regulatory_element"/>
    <tag k="subtype" v="traffic_light"/>
    <member type="way" ref="100" role="refers"/>
  </relation>
</osm>
```

## Testing

### Run All Tests
```bash
cd build
ctest --verbose
```

### Run Specific Test Suite
```bash
./build/hdmap_tests --gtest_filter=MapServerTest.*
./build/hdmap_tests --gtest_filter=RTreeTest.*
```

### Coverage Report (if enabled)
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
cmake --build .
ctest
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

## Project Structure

```
HDMap/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── include/                # Public headers
│   ├── types.hpp          # Core data structures
│   ├── rtree.hpp          # R-tree spatial index
│   ├── map_server.hpp     # Main API
│   └── lanelet2_parser.hpp # Map file parser
├── src/                    # Implementation
│   ├── types.cpp
│   ├── rtree.cpp
│   ├── map_server.cpp
│   ├── lanelet2_parser.cpp
│   └── main.cpp           # Demo application
├── tests/                  # Unit tests
│   ├── test_types.cpp
│   ├── test_rtree.cpp
│   └── test_map_server.cpp
├── data/                   # Sample map data
│   └── sample_map.osm
└── docs/                   # Additional documentation
```

## Skills Demonstrated

✅ **Embedded C++**: Memory-constrained design, explicit resource management
✅ **Spatial Algorithms**: R-tree implementation for efficient queries
✅ **API Design**: Clean, intuitive interface for autonomous driving
✅ **Safety**: RAII patterns, const correctness, bounds checking
✅ **Testing**: Comprehensive unit tests with Google Test
✅ **Build Systems**: Modern CMake with cross-compilation support
✅ **Performance**: O(log n) query complexity, memory-efficient structures

## Future Enhancements

- [ ] Route planning with A* algorithm
- [ ] Lane change feasibility checking
- [ ] Dynamic map updates (construction, closures)
- [ ] Multi-threading support for concurrent queries
- [ ] GPU acceleration for spatial queries
- [ ] Real-time map streaming from cloud
- [ ] Map diff/delta updates
- [ ] OpenDRIVE format support

## License

This is a hobby/portfolio project for demonstrating autonomous driving map server concepts.

## Author

Built as a technical demonstration project inspired by Woven by Toyota's in-vehicle map services.
