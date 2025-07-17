#!/bin/bash
# GPT4All Launcher Script with Debug

# Set environment variables for best compatibility
export QT_QPA_PLATFORM="xcb"
export QML_IMPORT_PATH="/usr/lib/aarch64-linux-gnu/qt6/qml"
export QT_AUTO_SCREEN_SCALE_FACTOR=0
export QT_DEBUG_PLUGINS=0

# Log file for debugging
LOGFILE="/tmp/gpt4all_launch.log"

# Function to log messages
log_msg() {
    echo "$(date): $1" >> "$LOGFILE"
    echo "$1"
}

# Clear previous log
> "$LOGFILE"

log_msg "Starting GPT4All launcher..."
log_msg "Display: $DISPLAY"
log_msg "Desktop session: $XDG_CURRENT_DESKTOP"

# Navigate to the GPT4All directory
cd "/media/jono/nvme/projects/gpt4all/gpt4all-chat/build/bin"

if [ ! -f "./chat" ]; then
    log_msg "ERROR: chat executable not found in $(pwd)"
    zenity --error --text="GPT4All executable not found!" 2>/dev/null || \
    xmessage "GPT4All executable not found!" 2>/dev/null || \
    notify-send "GPT4All Error" "Executable not found" 2>/dev/null
    exit 1
fi

log_msg "Launching GPT4All from $(pwd)..."

# Launch GPT4All with error handling
./chat 2>&1 | tee -a "$LOGFILE" &
PID=$!

# Wait a moment to see if it starts
sleep 3
if ! kill -0 $PID 2>/dev/null; then
    log_msg "GPT4All failed to start or crashed immediately"
    if command -v zenity >/dev/null 2>&1; then
        zenity --error --text="GPT4All failed to start. Check $LOGFILE for details."
    elif command -v xmessage >/dev/null 2>&1; then
        xmessage "GPT4All failed to start. Check $LOGFILE for details."
    elif command -v notify-send >/dev/null 2>&1; then
        notify-send "GPT4All Error" "Failed to start. Check $LOGFILE"
    fi
else
    log_msg "GPT4All started successfully with PID $PID"
fi
