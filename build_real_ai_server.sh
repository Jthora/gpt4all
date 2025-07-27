#!/bin/bash

# Build Real AI Server with GPT4All Integration
# This creates a server that actually connects to the GPT4All ML engine

echo "🔨 Building REAL AI Server with GPT4All ML Integration..."

cd /media/jono/nvme/projects/gpt4all-api-server

# Create build directory 
mkdir -p build_real_ai_server
cd build_real_ai_server

# Create main.cpp that uses the EnhancedAIServerFixed
cat > enhanced_ai_main.cpp << 'EOF'
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <csignal>

// Include the enhanced AI server
#include "../enhanced_ai_server_fixed.h"

static EnhancedAIServerFixed* g_server = nullptr;

void signalHandler(int signal) {
    Q_UNUSED(signal)
    qDebug() << "Received shutdown signal, stopping server...";
    if (g_server) {
        g_server->stopServer();
    }
    QCoreApplication::quit();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Set up signal handling
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    qDebug() << "🚀 Starting GPT4All Real AI Server...";
    
    // Create the enhanced AI server with real ML integration
    EnhancedAIServerFixed server;
    g_server = &server;
    
    // Start the server
    if (!server.startServer(4891)) {
        qCritical() << "❌ Failed to start server!";
        return 1;
    }
    
    qDebug() << "✅ Real AI Server started on http://localhost:4891";
    qDebug() << "📚 Endpoints:";
    qDebug() << "   POST /v1/chat/completions - Real GPT4All AI responses";
    qDebug() << "   GET  /v1/models - Available models";
    qDebug() << "🧠 Real ML integration: ENABLED";
    qDebug() << "🛑 Press Ctrl+C to stop";
    
    return app.exec();
}
EOF

# Create CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
project(RealAIServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Network)

# Enable MOC for Q_OBJECT
set(CMAKE_AUTOMOC ON)

# Add GPT4All includes
set(GPT4ALL_CHAT_DIR "../gpt4all-chat")
include_directories(${GPT4ALL_CHAT_DIR}/src)

# Create executable
add_executable(real_ai_server
    enhanced_ai_main.cpp
    ../enhanced_ai_server_fixed.cpp
)

# Link Qt6 libraries
target_link_libraries(real_ai_server 
    Qt6::Core 
    Qt6::Network
)

# Try to link GPT4All libraries if available
set(GPT4ALL_LIB_DIR "../build/bin")
if(EXISTS "${GPT4ALL_LIB_DIR}")
    target_link_directories(real_ai_server PRIVATE ${GPT4ALL_LIB_DIR})
    # Try to link core GPT4All library
    find_library(LLMODEL_LIB llmodel HINTS ${GPT4ALL_LIB_DIR})
    if(LLMODEL_LIB)
        target_link_libraries(real_ai_server ${LLMODEL_LIB})
        message(STATUS "Found GPT4All library: ${LLMODEL_LIB}")
    endif()
endif()

EOF

echo "📦 Configuring CMake..."
cmake .

if [ $? -eq 0 ]; then
    echo "🔧 Building..."
    make -j$(nproc)
    
    if [ $? -eq 0 ]; then
        echo "✅ Real AI Server built successfully!"
        echo "📁 Executable: ./real_ai_server"
        echo "🚀 Run with: ./real_ai_server"
    else
        echo "❌ Build failed"
    fi
else
    echo "❌ CMake configuration failed"
fi
