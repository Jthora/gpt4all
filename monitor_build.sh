#!/bin/bash
# GPT4All Build Progress Monitor

echo "🔍 GPT4All Build Progress Monitor"
echo "=================================="
echo ""

BUILD_DIR="/media/jono/nvme/projects/gpt4all/gpt4all-chat/build"

check_build_status() {
    echo "📊 Current Status:"
    echo "=================="
    
    if [ ! -d "$BUILD_DIR" ]; then
        echo "❌ Build directory doesn't exist"
        echo "🔧 Run: cd /media/jono/nvme/projects/gpt4all/gpt4all-chat && mkdir build && cd build"
        return 1
    fi
    
    cd "$BUILD_DIR"
    
    # Check if CMake is still running
    if pgrep -f cmake > /dev/null; then
        echo "🔄 CMake Configuration: RUNNING"
        echo "   Process: $(pgrep -f cmake)"
    else
        echo "✅ CMake Configuration: COMPLETED"
    fi
    
    # Check if Makefile exists
    if [ -f "Makefile" ]; then
        echo "✅ Build System: READY"
        
        # Check if binary exists
        if [ -f "bin/chat" ]; then
            echo "✅ GUI Application: BUILT"
            echo "🚀 Ready to launch: ./bin/chat"
        else
            echo "⏳ GUI Application: NEEDS COMPILATION"
            echo "🔧 Run: make -j4"
        fi
    else
        echo "⏳ Build System: WAITING FOR CMAKE"
        
        # Check download progress
        echo ""
        echo "📥 Download Progress:"
        if [ -d "deps" ]; then
            echo "   Deps directory: $(du -sh deps 2>/dev/null | cut -f1)"
        fi
        
        # Look for embedding model download
        EMBED_FILE=$(find . -name "*nomic-embed*" 2>/dev/null)
        if [ -n "$EMBED_FILE" ]; then
            echo "   Embedding model: $(ls -lh $EMBED_FILE 2>/dev/null | awk '{print $5}')"
        else
            echo "   Embedding model: Downloading..."
        fi
    fi
    
    echo ""
    echo "💾 Build Directory Size: $(du -sh . 2>/dev/null | cut -f1)"
    echo "⏱️  Last Activity: $(stat -c %y . 2>/dev/null | cut -d. -f1)"
}

monitor_loop() {
    echo "🔄 Monitoring build progress (Ctrl+C to stop)..."
    echo ""
    
    while true; do
        clear
        echo "🔍 GPT4All Build Progress Monitor - $(date)"
        echo "=============================================="
        echo ""
        
        check_build_status
        
        echo ""
        echo "⏱️  Next check in 30 seconds..."
        sleep 30
    done
}

# Main execution
if [ "$1" = "--monitor" ]; then
    monitor_loop
else
    check_build_status
    echo ""
    echo "💡 Run with --monitor for continuous monitoring"
fi
