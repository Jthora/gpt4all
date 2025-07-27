#!/bin/bash

# Standalone Enhanced AI Server Build Script
# Simple Qt-only build without GPT4All dependencies

echo "🔨 Building Standalone Enhanced AI Server..."

cd /media/jono/nvme/projects/gpt4all

# Create build directory
mkdir -p build_standalone_server
cd build_standalone_server

# Create CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
project(StandaloneEnhancedAIServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

# Enable MOC for Q_OBJECT
set(CMAKE_AUTOMOC ON)

# Create executable
add_executable(standalone_enhanced_ai_server
    ../standalone_enhanced_ai_server.cpp
)

# Link Qt6 libraries
target_link_libraries(standalone_enhanced_ai_server Qt6::Core Qt6::Network)

# Set output directory
set_target_properties(standalone_enhanced_ai_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ../gpt4all-chat/build/bin
)
EOF

# Build
cmake -DCMAKE_BUILD_TYPE=Release .
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo "✅ Standalone Enhanced AI Server built successfully!"
    echo "📁 Output: gpt4all-chat/build/bin/standalone_enhanced_ai_server"
    echo ""
    echo "🚀 To run the server:"
    echo "   cd /media/jono/nvme/projects/gpt4all/gpt4all-chat/build/bin"
    echo "   ./standalone_enhanced_ai_server"
    echo ""
    echo "🧪 Test with curl:"
    echo "   curl -X POST http://localhost:4891/v1/chat/completions \\"
    echo "     -H \"Content-Type: application/json\" \\"
    echo "     -d '{\"model\":\"gpt4all\",\"messages\":[{\"role\":\"user\",\"content\":\"Hello!\"}]}'"
else
    echo "❌ Build failed"
    exit 1
fi
