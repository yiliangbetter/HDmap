#!/bin/bash
set -e

# Script to download OSM benchmark data
# Usage: ./download_benchmark_data.sh [small|medium|large|xlarge]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
DATA_DIR="$PROJECT_DIR/test_data/benchmark"

mkdir -p "$DATA_DIR"
cd "$DATA_DIR"

SIZE="${1:-medium}"

case "$SIZE" in
    small)
        echo "Downloading Monaco (Small: ~2 MB uncompressed)..."
        URL="https://download.bbbike.org/osm/bbbike/Monaco/Monaco.osm.gz"
        FILENAME="Monaco.osm"
        COMPRESSED="Monaco.osm.gz"
        DECOMPRESS_CMD="gunzip"
        ;;
    medium)
        echo "Downloading San Francisco (Medium: ~150 MB uncompressed)..."
        URL="https://download.bbbike.org/osm/bbbike/SanFrancisco/SanFrancisco.osm.gz"
        FILENAME="SanFrancisco.osm"
        COMPRESSED="SanFrancisco.osm.gz"
        DECOMPRESS_CMD="gunzip"
        ;;
    large)
        echo "Downloading Tokyo (Large: ~500 MB uncompressed)..."
        URL="https://download.bbbike.org/osm/bbbike/Tokyo/Tokyo.osm.gz"
        FILENAME="Tokyo.osm"
        COMPRESSED="Tokyo.osm.gz"
        DECOMPRESS_CMD="gunzip"
        ;;
    xlarge)
        echo "Downloading New York (XLarge: ~800 MB uncompressed)..."
        URL="https://download.bbbike.org/osm/bbbike/NewYork/NewYork.osm.gz"
        FILENAME="NewYork.osm"
        COMPRESSED="NewYork.osm.gz"
        DECOMPRESS_CMD="gunzip"
        ;;
    *)
        echo "Usage: $0 [small|medium|large|xlarge]"
        exit 1
        ;;
esac

# Download if not already present
if [ -f "$FILENAME" ]; then
    echo "File $FILENAME already exists, skipping download."
else
    echo "Downloading from $URL..."
    curl -# -L -o "$COMPRESSED" "$URL"
    
    # Decompress
    if [ -f "$COMPRESSED" ]; then
        echo "Decompressing..."
        $DECOMPRESS_CMD -v "$COMPRESSED"
    else
        echo "Error: Download failed, compressed file not found"
        exit 1
    fi
fi

# Get file size
FILE_SIZE=$(du -h "$FILENAME" | cut -f1)
echo ""
echo "✓ Download complete!"
echo "  File: $DATA_DIR/$FILENAME"
echo "  Size: $FILE_SIZE"
echo ""
echo "To benchmark, run:"
echo "  time ./build/hdmap_server $DATA_DIR/$FILENAME"
