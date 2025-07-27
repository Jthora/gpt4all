#!/bin/bash

# Enhanced AI Server Build Script
# This script builds the complete enhanced AI server with real GPT4All integration

echo "🔨 Building Enhanced AI Server..."

# Change to project directory
cd /media/jono/nvme/projects/gpt4all

# Create build directory
mkdir -p build_enhanced_ai_server
cd build_enhanced_ai_server

# Configure CMake
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_COMPILER=g++ \
      -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6 \
      -f ../enhanced_ai_server_cmake.txt ..

# Build
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo "✅ Enhanced AI Server built successfully!"
    echo "📁 Output: gpt4all-chat/build/bin/enhanced_ai_server_fixed"
else
    echo "❌ Build failed"
    exit 1
fi
