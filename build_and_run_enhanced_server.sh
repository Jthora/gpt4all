#!/bin/bash

echo "🚀 Building Enhanced GPT4All SimpleServer..."

# Create build directory
mkdir -p build_enhanced_server
cd build_enhanced_server

# Use cmake with the existing CMake setup but focus on our server
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
project(GPT4AllEnhancedServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

set(CMAKE_AUTOMOC ON)

add_executable(run_simple_server 
    ../run_simple_server.cpp
    ../gpt4all-chat/src/simpleserver.cpp
)

target_link_libraries(run_simple_server 
    Qt6::Core 
    Qt6::Network
)

target_include_directories(run_simple_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../gpt4all-chat/src
)
EOF

# Build
echo "🔨 Configuring build..."
cmake . 

echo "🔨 Building..."
make -j4

if [ -f run_simple_server ]; then
    echo "✅ Build successful!"
    echo "🚀 Starting enhanced GPT4All SimpleServer..."
    echo ""
    ./run_simple_server
else
    echo "❌ Build failed"
    exit 1
fi
