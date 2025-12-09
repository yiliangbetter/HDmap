#!/bin/bash
# Build script for HD Map Server

set -e

echo "=== Building HD Map Server ==="

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)

echo ""
echo "=== Build Complete ==="
echo "Executable: build/hdmap_server"
echo "Tests: build/hdmap_tests"
echo ""
echo "Run with: ./build/hdmap_server data/sample_map.osm"
echo "Test with: ./build/hdmap_tests"
