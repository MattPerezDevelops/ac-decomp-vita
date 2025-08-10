#!/bin/bash

# Animal Crossing Vita Port - Clean Build Script
# Streamlined approach for AC-Decomp integration

set -e

echo "🎮 Animal Crossing Vita - Clean Build System"
echo "============================================="

# Configuration
DOCKER_IMAGE="ac-vita-clean"
BUILD_MODE="${1:-demo}"  # demo or full

# Build Docker image if it doesn't exist
if ! docker images | grep -q "$DOCKER_IMAGE"; then
    echo "🐳 Building Docker environment..."
    docker build -f Dockerfile.clean -t "$DOCKER_IMAGE" .
fi

case "$BUILD_MODE" in
    "demo")
        echo "🎮 Building demo mode (platform test)"
        docker run --rm \
            -v "$(pwd)/..:/workspace" \
            -w "/workspace/vita" \
            "$DOCKER_IMAGE" \
            bash -c "
                mkdir -p build && cd build
                cmake .. -DAC_DEMO_MODE=ON -DAC_INCLUDE_DECOMP=OFF
                make -j4
                echo '✅ Demo VPK ready: build/AnimalCrossingVita.vpk'
            "
        ;;
        
    "full")
        echo "🎮 Building full game (AC-Decomp integration)"
        docker run --rm \
            -v "$(pwd)/..:/workspace" \
            -w "/workspace/vita" \
            "$DOCKER_IMAGE" \
            bash -c "
                mkdir -p build && cd build
                cmake .. -DAC_DEMO_MODE=OFF -DAC_INCLUDE_DECOMP=ON
                make -j4 2>&1 | tee ../ac_build.log
                echo '✅ Full game VPK ready: build/AnimalCrossingVita.vpk'
            "
        ;;
        
    "assets")
        echo "ℹ️  Asset conversion not needed!"
        echo "AC-Decomp handles assets at compile-time using #include statements."
        echo "Just ensure you have original game files in ../dump/ directory:"
        echo "  - static.dol (renamed from main.dol)"
        echo "  - forest_1st.arc"
        echo "  - forest_2nd.arc"
        echo "  - foresta.rel.szs"
        ;;
        
    "clean")
        echo "🧹 Cleaning build directory..."
        rm -rf build/ ac_build.log
        echo "✅ Clean complete"
        ;;
        
    *)
        echo "Usage: $0 [demo|full|assets|clean]"
        echo ""
        echo "  demo   - Build platform test demo"
        echo "  full   - Build complete AC-Decomp integration"
        echo "  assets - Convert GameCube assets to Vita format"  
        echo "  clean  - Clean build directory"
        exit 1
        ;;
esac

echo "�� Build complete!" 