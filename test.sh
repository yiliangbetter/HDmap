#!/bin/bash
# Test script for HD Map Server

set -e

echo "=== Running HD Map Server Tests ==="

# Build if needed
if [ ! -f "build/hdmap_tests" ]; then
    echo "Tests not built. Building first..."
    bash build.sh
fi

cd build

# Run tests
echo ""
echo "Running unit tests..."
./hdmap_tests

echo ""
echo "=== All Tests Passed ==="
