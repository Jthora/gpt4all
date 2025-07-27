#!/bin/bash

# CUDA Functionality Verification for GPT4All
# Tests if CUDA acceleration is working after Qt downgrade

echo "🚀 GPT4All CUDA Functionality Test"
echo "=================================="

cd /media/jono/nvme/projects/gpt4all/gpt4all-chat/build/bin

echo "📋 System Information:"
echo "• CUDA Libraries Available:"
ls -la libllamamodel-mainline-cuda*.so 2>/dev/null || echo "  ❌ No CUDA libraries found"

echo ""
echo "• Library Dependencies:"
ldd chat | grep -i cuda || echo "  ℹ️  No direct CUDA dependencies in main binary"

echo ""
echo "🧪 Testing Model Backend Detection:"

# Test if the application can detect available backends
echo "• Starting GPT4All to check backend detection..."
timeout 10 ./chat 2>&1 | {
    while IFS= read -r line; do
        case "$line" in
            *cuda*|*CUDA*|*gpu*|*GPU*)
                echo "  ✅ CUDA: $line"
                ;;
            *kompute*|*Kompute*)
                echo "  ⚠️  Kompute (disabled): $line"
                ;;
            *backend*|*Backend*)
                echo "  ℹ️  Backend: $line"
                ;;
            *device*|*Device*)
                echo "  ℹ️  Device: $line"
                ;;
            *ERROR*|*error*)
                echo "  ❌ Error: $line"
                ;;
        esac
    done
}

echo ""
echo "🔍 CUDA Runtime Analysis:"

# Check if CUDA runtime is available on the system
if command -v nvidia-smi &> /dev/null; then
    echo "• NVIDIA Driver Status:"
    nvidia-smi --query-gpu=name,memory.total,utilization.gpu --format=csv,noheader,nounits 2>/dev/null || echo "  ⚠️  nvidia-smi available but no GPU detected"
else
    echo "  ⚠️  nvidia-smi not available - GPU status unknown"
fi

# Check CUDA toolkit
if [ -d "/usr/local/cuda" ]; then
    echo "• CUDA Toolkit: Found at /usr/local/cuda"
    if [ -f "/usr/local/cuda/version.txt" ]; then
        echo "  Version: $(cat /usr/local/cuda/version.txt)"
    fi
else
    echo "• CUDA Toolkit: Not found at standard location"
fi

echo ""
echo "🎯 Recommendations:"

# Check library linking
if ldd chat | grep -q cuda; then
    echo "✅ CUDA libraries appear to be linked"
else
    echo "ℹ️  CUDA libraries are dynamically loaded (normal for llama.cpp)"
fi

echo ""
echo "📝 To verify CUDA is working:"
echo "1. Load a model in GPT4All GUI"
echo "2. Check GPU utilization with: watch -n 1 nvidia-smi"
echo "3. Compare response times with/without GPU"
echo "4. Look for GPU memory allocation in nvidia-smi"

echo ""
echo "🔧 If CUDA isn't working:"
echo "1. Check NVIDIA drivers: nvidia-smi"
echo "2. Verify CUDA runtime: ldconfig -p | grep cuda"
echo "3. Test with a simple CUDA program"
echo "4. Check model format supports GPU acceleration"
