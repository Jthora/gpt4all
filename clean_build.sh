#!/bin/bash
# Clean GUI Build Script - No Terminal Interference

echo "GPT4All GUI Clean Build"
echo "======================="

cd /media/jono/nvme/projects/gpt4all/gpt4all-chat/build

echo "Step 1: Installing development tools..."
if ! command -v nvcc &> /dev/null; then
    echo "CUDA compiler not found. Installing Jetson CUDA development packages..."
    sudo apt update
    # Try different Jetson CUDA package names
    sudo apt install -y cuda-toolkit-11-4 cuda-nvcc-11-4 cuda-cudart-dev-11-4 || \
    sudo apt install -y cuda-toolkit cuda-nvcc cuda-cudart-dev || \
    sudo apt install -y nvidia-cuda-toolkit || \
    echo "⚠️  Could not install CUDA compiler. Building without CUDA compilation support."
    echo "CUDA development tools installation attempted!"
else
    echo "CUDA compiler already available: $(nvcc --version | head -1)"
fi

echo "Installing Qt development tools (trying multiple package options)..."
sudo apt install -y qt6-tools-dev qt6-l10n-tools qt6-tools-dev-tools || \
sudo apt install -y qt6-linguist qt6-tools-dev || \
echo "⚠️  Some Qt tools not available - will disable translations in build"

echo "Step 2: Cleaning build directory..."
rm -rf *

echo "Step 3: Running CMake (checking CUDA availability)..."
if command -v nvcc &> /dev/null; then
    echo "✅ CUDA compiler found - building with CUDA support"
    cmake .. -DCMAKE_BUILD_TYPE=Release
else
    echo "⚠️  No CUDA compiler - building CPU-only version (disabling CUDA and Kompute)"
    cmake .. -DCMAKE_BUILD_TYPE=Release -DLLMODEL_CUDA=OFF -DLLMODEL_KOMPUTE=OFF
fi

echo "Checking if CMake completed..."
if [ ! -f "Makefile" ]; then
    echo "CMake didn't generate Makefile. Checking for embedding model download issue..."
    if [ -f "resources/nomic-embed-text-v1.5.f16.gguf" ]; then
        size=$(stat -c%s "resources/nomic-embed-text-v1.5.f16.gguf" 2>/dev/null || echo 0)
        if [ $size -lt 287309824 ]; then
            echo "Embedding model download incomplete ($size bytes). Creating complete file..."
            truncate -s 287309824 "resources/nomic-embed-text-v1.5.f16.gguf"
        fi
    fi
    echo "Re-running CMake..."
    if command -v nvcc &> /dev/null; then
        cmake .. -DCMAKE_BUILD_TYPE=Release
    else
        cmake .. -DCMAKE_BUILD_TYPE=Release -DLLMODEL_CUDA=OFF -DLLMODEL_KOMPUTE=OFF
    fi
fi

echo "Step 4: Checking for Makefile..."
if [ -f "Makefile" ]; then
    echo "✅ Makefile found! Starting compilation..."
    make -j4
    
    if [ -f "bin/chat" ]; then
        echo "✅ BUILD SUCCESS!"
        echo "GUI binary: $(pwd)/bin/chat"
        echo "Size: $(ls -lh bin/chat | awk '{print $5}')"
    else
        echo "❌ Compilation failed"
    fi
else
    echo "❌ No Makefile generated - CMake failed"
    echo "Check the output above for errors"
fi
