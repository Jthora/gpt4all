#!/bin/bash
# Complete GPT4All GUI Build & Installation Script

echo "🚀 GPT4All GUI Complete Build & Desktop Integration"
echo "==================================================="

# Manual build instructions due to terminal limitations
echo ""
echo "📋 MANUAL BUILD INSTRUCTIONS:"
echo "=============================="
echo ""
echo "1. 📁 Navigate to build directory:"
echo "   cd /media/jono/nvme/projects/gpt4all/gpt4all-chat/build"
echo ""
echo "2. 🔧 Configure with CMake:"
echo "   rm -f CMakeCache.txt"
echo "   cmake .. -DCMAKE_BUILD_TYPE=Release -DLLMODEL_KOMPUTE=OFF -DLLMODEL_CUDA=OFF"
echo ""
echo "3. 🔨 Compile (takes 10-15 minutes):"
echo "   make -j4"
echo ""
echo "4. ✅ Verify build:"
echo "   ls -la bin/chat"
echo ""

# Install desktop integration
echo "🖥️  DESKTOP INTEGRATION:"
echo "========================"
echo ""

# Create applications directory if needed
mkdir -p ~/.local/share/applications

# Copy desktop file
cp /media/jono/nvme/projects/gpt4all/gpt4all.desktop ~/.local/share/applications/

# Make it executable
chmod +x ~/.local/share/applications/gpt4all.desktop

# Update desktop database
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database ~/.local/share/applications/ 2>/dev/null
    echo "✅ Desktop database updated"
fi

echo "✅ Desktop entry installed!"
echo ""
echo "🎯 POST-BUILD ACTIONS:"
echo "======================"
echo ""
echo "After successful compilation, you can:"
echo ""
echo "1. 🖱️  Launch from Applications Menu:"
echo "   • Look for 'GPT4All' in Development/Education categories"
echo "   • Click to launch the GUI"
echo ""
echo "2. ⌨️  Launch from Terminal:"
echo "   cd /media/jono/nvme/projects/gpt4all/gpt4all-chat/build/bin"
echo "   ./chat"
echo ""
echo "3. 📂 Create Desktop Shortcut (optional):"
echo "   cp ~/.local/share/applications/gpt4all.desktop ~/Desktop/"
echo "   chmod +x ~/Desktop/gpt4all.desktop"
echo ""

# Create a simple launcher for convenience
echo "🎯 Creating convenient launcher..."
cat > /media/jono/nvme/projects/gpt4all/start_gui.sh << 'EOF'
#!/bin/bash
# GPT4All GUI Quick Launcher

echo "🚀 Starting GPT4All GUI..."

# Check if GUI is built
if [ ! -f "/media/jono/nvme/projects/gpt4all/gpt4all-chat/build/bin/chat" ]; then
    echo "❌ GUI not built yet!"
    echo "Please run the build commands first:"
    echo "   cd /media/jono/nvme/projects/gpt4all/gpt4all-chat/build"
    echo "   cmake .. -DCMAKE_BUILD_TYPE=Release -DLLMODEL_KOMPUTE=OFF -DLLMODEL_CUDA=OFF"
    echo "   make -j4"
    exit 1
fi

# Launch GUI
cd /media/jono/nvme/projects/gpt4all/gpt4all-chat/build/bin
./chat
EOF

chmod +x /media/jono/nvme/projects/gpt4all/start_gui.sh

echo "✅ Quick launcher created: /media/jono/nvme/projects/gpt4all/start_gui.sh"
echo ""
echo "🔧 TROUBLESHOOTING:"
echo "==================="
echo ""
echo "If compilation fails:"
echo "• Check Qt development packages: sudo apt install qt6-base-dev"
echo "• Verify C++ compiler: gcc --version"
echo "• Check available disk space: df -h"
echo ""
echo "If GUI doesn't start:"
echo "• Ensure X11 forwarding: export DISPLAY=:0"
echo "• Check permissions: chmod +x bin/chat"
echo "• Verify dependencies: ldd bin/chat"
echo ""
echo "📚 NEXT STEPS:"
echo "=============="
echo "1. Execute the manual build commands above"
echo "2. Test the GUI: ./start_gui.sh"
echo "3. Download a model when prompted (orca-mini-3b recommended)"
echo "4. Enjoy your local AI chat interface!"
echo ""
echo "🎉 Setup complete! Ready for manual build execution."
