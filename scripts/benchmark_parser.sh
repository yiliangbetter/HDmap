#!/bin/bash
set -e

# Performance benchmark script for Lanelet2 parser
# Usage: ./benchmark_parser.sh <osm_file>

if [ $# -eq 0 ]; then
    echo "Usage: $0 <osm_file>"
    echo "Example: $0 test_data/benchmark/berlin-latest.osm"
    exit 1
fi

OSM_FILE="$1"

if [ ! -f "$OSM_FILE" ]; then
    echo "Error: File not found: $OSM_FILE"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
EXECUTABLE="$PROJECT_DIR/build/hdmap_server"

if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable not found. Please build first with: bash build.sh"
    exit 1
fi

# Get file info
FILE_SIZE=$(du -h "$OSM_FILE" | cut -f1)
LINE_COUNT=$(wc -l < "$OSM_FILE" | tr -d ' ')

echo "========================================="
echo "HD Map Parser Performance Benchmark"
echo "========================================="
echo "File: $OSM_FILE"
echo "Size: $FILE_SIZE"
echo "Lines: $LINE_COUNT"
echo ""
echo "Starting benchmark..."
echo "-----------------------------------------"

# Run with time measurement
# Using /usr/bin/time for more detailed stats on macOS
if command -v gtime &> /dev/null; then
    # Use GNU time if available (brew install gnu-time)
    echo "Using GNU time for detailed statistics"
    /usr/bin/gtime -v "$EXECUTABLE" "$OSM_FILE" 2>&1 | tee benchmark_results.txt
else
    # Fallback to built-in time
    echo "Using built-in time command"
    { time "$EXECUTABLE" "$OSM_FILE" ; } 2>&1 | tee benchmark_results.txt
fi

echo ""
echo "-----------------------------------------"
echo "✓ Benchmark complete!"
echo "Results saved to: benchmark_results.txt"
echo ""

# Parse and display key metrics if available
if grep -q "real" benchmark_results.txt; then
    echo "Performance Summary:"
    grep "real\|user\|sys" benchmark_results.txt
fi
