#!/bin/bash
# GPT4All GUI Build and Desktop Integration Script

echo "🚀 GPT4All GUI Builder & Desktop Integration"
echo "============================================="
echo ""

# Set build directory
BUILD_DIR="/media/jono/nvme/projects/gpt4all/gpt4all-chat/build"
CHAT_SOURCE_DIR="/media/jono/nvme/projects/gpt4all/gpt4all-chat"
DESKTOP_FILE="$HOME/.local/share/applications/gpt4all.desktop"
ICON_DIR="$HOME/.local/share/icons"

# Step 1: Navigate to build directory
echo "📁 Step 1: Setting up build environment..."
cd "$BUILD_DIR" || { echo "❌ Build directory not found"; exit 1; }
pwd

# Step 2: Clean and configure with CMake
echo ""
echo "🔧 Step 2: Configuring build with CMake..."
rm -f CMakeCache.txt
cmake .. -DCMAKE_BUILD_TYPE=Release -DLLMODEL_KOMPUTE=OFF -DLLMODEL_CUDA=OFF

# Check if Makefile was generated
if [ ! -f "Makefile" ]; then
    echo "❌ CMake configuration failed - no Makefile generated"
    exit 1
fi
echo "✅ CMake configuration successful!"

# Step 3: Build the GUI application
echo ""
echo "🔨 Step 3: Compiling GUI application..."
echo "⏳ This will take 10-15 minutes on Jetson AGX Orin..."
make -j4

# Check if binary was created
if [ ! -f "bin/chat" ]; then
    echo "❌ Compilation failed - no chat binary found"
    exit 1
fi
echo "✅ GUI application compiled successfully!"

# Step 4: Create desktop icon
echo ""
echo "🖼️  Step 4: Creating desktop integration..."

# Create icon directory if it doesn't exist
mkdir -p "$ICON_DIR"

# Create a simple icon (you can replace this with a better icon later)
# For now, we'll use a system icon or create a simple text-based icon
ICON_PATH="$ICON_DIR/gpt4all.png"

# Try to find or create an icon
if command -v convert >/dev/null 2>&1; then
    # Create a simple icon using ImageMagick if available
    convert -size 64x64 xc:blue -font DejaVu-Sans-Bold -pointsize 12 -fill white \
        -gravity center -annotate +0+0 'GPT4All' "$ICON_PATH" 2>/dev/null || \
        cp /usr/share/pixmaps/utilities-terminal.png "$ICON_PATH" 2>/dev/null || \
        echo "⚠️  Using system icon fallback"
else
    # Use a system icon as fallback
    cp /usr/share/pixmaps/utilities-terminal.png "$ICON_PATH" 2>/dev/null || \
        cp /usr/share/pixmaps/applications-office.png "$ICON_PATH" 2>/dev/null || \
        echo "⚠️  No icon available - will use system default"
fi

# Create desktop entry
cat > "$DESKTOP_FILE" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=GPT4All
Comment=Local AI Chat Interface
Exec=$BUILD_DIR/bin/chat
Icon=$ICON_PATH
Terminal=false
Categories=Development;Education;Science;
StartupNotify=true
StartupWMClass=chat
Keywords=AI;Chat;Language;Model;GPT;
EOF

# Make desktop file executable
chmod +x "$DESKTOP_FILE"

# Update desktop database
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$HOME/.local/share/applications/" 2>/dev/null || true
fi

echo "✅ Desktop integration complete!"

# Step 5: Create launcher script
echo ""
echo "🎯 Step 5: Creating convenient launcher..."

LAUNCHER_SCRIPT="/media/jono/nvme/projects/gpt4all/launch_gpt4all_gui.sh"
cat > "$LAUNCHER_SCRIPT" << EOF
#!/bin/bash
# GPT4All GUI Launcher

echo "🚀 Starting GPT4All GUI..."
cd "$BUILD_DIR/bin"
./chat
EOF

chmod +x "$LAUNCHER_SCRIPT"

echo "✅ Launcher script created!"

# Final summary
echo ""
echo "🎉 BUILD COMPLETE!"
echo "=================="
echo ""
echo "GUI Application: $BUILD_DIR/bin/chat"
echo "Desktop Entry: $DESKTOP_FILE"
echo "Launcher Script: $LAUNCHER_SCRIPT"
echo ""
echo "💡 How to use:"
echo "1. 🖱️  Click 'GPT4All' in your applications menu"
echo "2. ⌨️  Run: $LAUNCHER_SCRIPT"
echo "3. 📂 Navigate to: $BUILD_DIR/bin && ./chat"
echo ""
echo "🔧 Troubleshooting:"
echo "• If GUI doesn't start, check: export DISPLAY=:0"
echo "• For missing models, the app will prompt to download"
echo "• CUDA acceleration is available if models support it"
echo ""
echo "✅ Ready to use GPT4All GUI!"
