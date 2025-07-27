#!/bin/bash
# Simple GPT4All GUI Build Script

echo "Building GPT4All GUI..."

# Navigate to build directory
cd /media/jono/nvme/projects/gpt4all/gpt4all-chat/build

# Clean previous build
rm -f CMakeCache.txt

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release -DLLMODEL_KOMPUTE=OFF -DLLMODEL_CUDA=OFF

# Build
make -j4

echo "Build complete! Check for bin/chat"
