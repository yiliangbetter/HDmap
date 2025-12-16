#!/bin/bash
# Script to monitor runtime memory usage of hdmap_server

echo "=== Runtime Memory Analysis for HDMap Server ==="
echo ""

# Check if binary exists
if [ ! -f "build/hdmap_server" ]; then
    echo "Error: build/hdmap_server not found. Build the project first."
    exit 1
fi

# Run the program in background and capture its PID
echo "Starting hdmap_server..."
./build/hdmap_server test_data/benchmark/SanFrancisco.osm &
PID=$!

# Wait a moment for it to load
sleep 2

# Check if process is still running
if ! ps -p $PID > /dev/null; then
    echo "Process finished too quickly. Output from program:"
    wait $PID
    exit 0
fi

echo ""
echo "Process ID: $PID"
echo ""

# Get memory information using ps
echo "=== Memory Usage (ps) ==="
ps -p $PID -o pid,rss,vsz,comm | head -2
echo ""

# RSS is in KB, convert to MB
RSS_KB=$(ps -p $PID -o rss= | tr -d ' ')
RSS_MB=$(echo "scale=2; $RSS_KB / 1024" | bc)
VSZ_KB=$(ps -p $PID -o vsz= | tr -d ' ')
VSZ_MB=$(echo "scale=2; $VSZ_KB / 1024" | bc)

echo "Real Memory (RSS): ${RSS_MB} MB"
echo "Virtual Memory (VSZ): ${VSZ_MB} MB"
echo ""

# Get detailed memory map using vmmap
echo "=== Memory Segments (vmmap) ==="
vmmap $PID 2>/dev/null | grep -E "REGION|STACK|MALLOC|__DATA|__TEXT" | head -20
echo ""

# Get heap statistics
echo "=== Heap Allocations (heap) ==="
heap $PID 2>/dev/null | grep -E "TOTAL|malloc|All zones" | head -10
echo ""

# Let the process finish
echo "Waiting for process to complete..."
wait $PID

echo ""
echo "=== Analysis Complete ==="
