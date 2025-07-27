#!/bin/bash

# Simple Enhanced AI Server Build Script
# Uses existing GPT4All build environment

echo "🔨 Building Enhanced AI Server using existing GPT4All build..."

# Change to project directory
cd /media/jono/nvme/projects/gpt4all

# Create simple standalone build
echo "📦 Creating standalone enhanced AI server..."

# Get Qt6 paths from existing build
export QT_DIR=$(find /usr -name "Qt6Config.cmake" -path "*/lib/*" -printf "%h\n" | head -1)
export CMAKE_PREFIX_PATH="$QT_DIR"
export PKG_CONFIG_PATH="/usr/lib/x86_64-linux-gnu/pkgconfig"

# Create build directory
mkdir -p build_enhanced_ai_server
cd build_enhanced_ai_server

# Create simplified CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
project(EnhancedAIServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Sql Widgets)

# Set up for MOC
set(CMAKE_AUTOMOC ON)

# Source files
set(SOURCES
    ../enhanced_ai_server_app.cpp
    ../enhanced_ai_server.cpp
)

# Create executable
add_executable(enhanced_ai_server_fixed ${SOURCES})

# Link Qt6 libraries
target_link_libraries(enhanced_ai_server_fixed Qt6::Core Qt6::Network Qt6::Sql Qt6::Widgets)

# Include directories
target_include_directories(enhanced_ai_server_fixed PRIVATE
    ../gpt4all-chat/src
    ../gpt4all-backend
)

# Try to find and link GPT4All libraries
find_library(LLMODEL_LIB llmodel PATHS ../gpt4all-backend/build ../gpt4all-chat/build/bin)
if(LLMODEL_LIB)
    target_link_libraries(enhanced_ai_server_fixed ${LLMODEL_LIB})
endif()

# Set output directory
set_target_properties(enhanced_ai_server_fixed PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ../gpt4all-chat/build/bin
)
EOF

# Configure and build
cmake -DCMAKE_BUILD_TYPE=Release .
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo "✅ Enhanced AI Server built successfully!"
    echo "📁 Output: gpt4all-chat/build/bin/enhanced_ai_server_fixed"
    echo ""
    echo "🚀 To run the server:"
    echo "   cd /media/jono/nvme/projects/gpt4all/gpt4all-chat/build/bin"
    echo "   ./enhanced_ai_server_fixed"
else
    echo "❌ Build failed"
    exit 1
fi
