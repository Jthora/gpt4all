#!/bin/bash

# GPT4All Real AI Server - Working Implementation Setup
echo "🚀 Setting up REAL GPT4All LLM Inference..."

# Create symlinks for backend discovery in current directory
echo "📚 Creating backend library symlinks..."
ln -sf ./build/bin/libllamamodel-mainline-cpu.so .
ln -sf ./build/bin/libllmodel.so .
ln -sf ./build/bin/libllmodel.so.0 .
ln -sf ./build/bin/libllmodel.so.0.5.0 .

# Verify backend libraries exist
echo "🔍 Verifying backend libraries..."
ls -la ./build/bin/libllamamodel-mainline-cpu.so ./build/bin/libllmodel.so*

# Set up environment for real inference
export LD_LIBRARY_PATH="./build/bin:$LD_LIBRARY_PATH"
export GPT4ALL_IMPLEMENTATIONS_PATH="./build/bin"

echo "✅ Environment configured for real AI inference"
echo "📊 Available models:"
ls -la /media/jono/nvme/models/gpt4all/*.gguf | head -3

echo "🔥 Starting REAL AI server..."
./real_gpt4all_server_microhttpd
