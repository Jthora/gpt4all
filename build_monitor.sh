#!/bin/bash
# Real-time build monitor

echo "🔍 GPT4All Build Monitor - Real Time"
echo "===================================="

while true; do
    clear
    echo "🔍 GPT4All Build Monitor - $(date)"
    echo "===================================="
    echo ""
    
    # Check build directory
    echo "📁 Build Directory Status:"
    pwd
    echo ""
    
    # Check CMake files
    if [ -f "CMakeCache.txt" ]; then
        echo "✅ CMake Cache: EXISTS"
        cache_size=$(ls -lh CMakeCache.txt | awk '{print $5}')
        echo "   Size: $cache_size"
    else
        echo "❌ CMake Cache: MISSING"
    fi
    
    # Check Makefile
    if [ -f "Makefile" ]; then
        echo "✅ Makefile: EXISTS!"
        echo "   🎯 Ready to compile with: make -j4"
        break
    else
        echo "⏳ Makefile: NOT READY"
    fi
    
    # Check embedding model download
    if [ -f "resources/nomic-embed-text-v1.5.f16.gguf" ]; then
        size=$(ls -lh resources/nomic-embed-text-v1.5.f16.gguf | awk '{print $5}')
        echo "📦 Embedding Model: $size"
        if [ "$size" = "274M" ]; then
            echo "   ✅ Download complete!"
        else
            echo "   ⏳ Still downloading..."
        fi
    else
        echo "📦 Embedding Model: NOT FOUND"
    fi
    
    # Check CMake process
    cmake_proc=$(ps aux | grep cmake | grep -v grep | wc -l)
    if [ $cmake_proc -gt 0 ]; then
        echo "⚡ CMake Process: RUNNING"
    else
        echo "💤 CMake Process: NOT RUNNING"
    fi
    
    echo ""
    echo "⏱️  Checking again in 10 seconds... (Ctrl+C to stop)"
    sleep 10
done

echo ""
echo "🎉 READY TO BUILD!"
echo "=================="
echo "Run: make -j4"
