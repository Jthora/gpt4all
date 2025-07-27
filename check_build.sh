#!/bin/bash
# Build Progress Monitor

echo "🔍 GPT4All GUI Build Progress Monitor"
echo "===================================="

BUILD_DIR="/media/jono/nvme/projects/gpt4all/gpt4all-chat/build"
cd "$BUILD_DIR"

echo "📁 Current directory: $(pwd)"
echo ""

echo "📋 Build Status Check:"
echo "----------------------"

# Check if CMake ran
if [ -f "CMakeCache.txt" ]; then
    echo "✅ CMake configuration: COMPLETE"
    cmake_time=$(stat -c %Y CMakeCache.txt)
    echo "   Last configured: $(date -d @$cmake_time)"
else
    echo "❌ CMake configuration: MISSING"
    echo "   Run: cmake .. -DCMAKE_BUILD_TYPE=Release -DLLMODEL_KOMPUTE=OFF -DLLMODEL_CUDA=OFF"
fi

# Check if Makefile exists
if [ -f "Makefile" ]; then
    echo "✅ Makefile: EXISTS"
else
    echo "❌ Makefile: MISSING"
    echo "   CMake configuration needed first"
fi

# Check compilation progress
echo ""
echo "🔨 Compilation Status:"
echo "----------------------"

if [ -d "bin" ]; then
    echo "✅ Binary directory: EXISTS"
    if [ -f "bin/chat" ]; then
        echo "✅ GUI Application: BUILT!"
        size=$(ls -lh bin/chat | awk '{print $5}')
        echo "   Size: $size"
        echo "   Path: $BUILD_DIR/bin/chat"
    else
        echo "⏳ GUI Application: IN PROGRESS"
        echo "   Binary directory exists but chat not ready"
    fi
else
    echo "⏳ Binary directory: NOT CREATED YET"
    echo "   Compilation not started or failed"
fi

# Check for object files (compilation in progress)
obj_count=$(find . -name "*.o" 2>/dev/null | wc -l)
if [ $obj_count -gt 0 ]; then
    echo "⚡ Object files found: $obj_count (compilation active)"
else
    echo "💤 No object files found (compilation not active)"
fi

echo ""
echo "📊 Current Build Files:"
echo "----------------------"
ls -la | head -10

echo ""
echo "🎯 Next Steps:"
echo "==============" 

if [ ! -f "Makefile" ]; then
    echo "1. Run CMake configuration:"
    echo "   cd $BUILD_DIR"
    echo "   cmake .. -DCMAKE_BUILD_TYPE=Release -DLLMODEL_KOMPUTE=OFF -DLLMODEL_CUDA=OFF"
elif [ ! -f "bin/chat" ]; then
    echo "1. Start compilation:"
    echo "   cd $BUILD_DIR"
    echo "   make -j4"
    echo ""
    echo "   ⏱️  Expected time: 10-15 minutes"
    echo "   💾 Expected size: ~50-100MB"
else
    echo "🎉 BUILD COMPLETE!"
    echo "   Launch: $BUILD_DIR/bin/chat"
    echo "   Desktop integration ready!"
fi
