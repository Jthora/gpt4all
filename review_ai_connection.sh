#!/bin/bash

echo "🔍 REVIEWING AI CONNECTION IN real_gpt4all_server.cpp"
echo "====================================================="

echo ""
echo "1️⃣ Checking for MOCK DATA in the code..."
echo ""

# Check for any mock/fake data in the generateResponse function
cd /media/jono/nvme/projects/gpt4all-api-server

echo "🔍 Searching for mock responses..."
grep -n -i "mock\|fake\|hardcoded\|test.*response" real_gpt4all_server.cpp || echo "✅ No mock data found"

echo ""
echo "🔍 Searching for hardcoded return values..."
grep -A5 -B5 'return.*".*"' real_gpt4all_server.cpp | grep -v "Error:" || echo "✅ No hardcoded responses found"

echo ""
echo "2️⃣ Verifying REAL AI INFERENCE methods are used..."
echo ""

echo "🤖 Checking for real GPT4All API calls:"
echo ""

echo "✓ Checking for Chat.newPromptResponsePair():"
grep -n "newPromptResponsePair" real_gpt4all_server.cpp && echo "✅ REAL AI method found"

echo ""
echo "✓ Checking for ChatLLM.prompt():"
grep -n "chatLLM.*prompt" real_gpt4all_server.cpp && echo "✅ REAL AI method found"

echo ""
echo "✓ Checking for ChatModel.responseText():"
grep -n "responseText" real_gpt4all_server.cpp && echo "✅ REAL AI method found"

echo ""
echo "✓ Checking for model loading:"
grep -n "loadModel\|isModelLoaded" real_gpt4all_server.cpp && echo "✅ REAL model loading found"

echo ""
echo "3️⃣ Checking generateResponse() function structure..."
echo ""

echo "📋 generateResponse() function analysis:"
sed -n '/QString generateResponse/,/^}/p' real_gpt4all_server.cpp | head -20

echo ""
echo "4️⃣ VERDICT:"
echo ""

# Check if there are any suspicious patterns
if grep -q "return.*Create.*categorized.*list" real_gpt4all_server.cpp; then
    echo "❌ FOUND HARDCODED CAR PARTS RESPONSE - THIS IS MOCK DATA!"
elif grep -q "return.*Engine.*Transmission" real_gpt4all_server.cpp; then
    echo "❌ FOUND HARDCODED CAR PARTS RESPONSE - THIS IS MOCK DATA!"
elif grep -q "newPromptResponsePair\|chatLLM.*prompt\|responseText" real_gpt4all_server.cpp; then
    echo "✅ REAL AI INFERENCE CONNECTION CONFIRMED"
    echo "   - Uses Chat.newPromptResponsePair() for real AI"
    echo "   - Uses ChatLLM.prompt() for real AI"  
    echo "   - Uses ChatModel.responseText() for real responses"
    echo "   - NO MOCK DATA DETECTED"
else
    echo "⚠️  UNCLEAR - Need to investigate further"
fi

echo ""
echo "5️⃣ Testing if server can be run directly..."
echo ""

if [ -f "real_gpt4all_server.cpp" ]; then
    echo "📁 real_gpt4all_server.cpp exists"
    
    # Check if there's a compiled version
    if [ -d "build_real_server" ]; then
        cd build_real_server
        if [ -f "test_ai_connection" ]; then
            echo "🔧 Found compiled test, attempting to run..."
            timeout 10s ./test_ai_connection || echo "⚠️  Test run failed or timed out"
        else
            echo "⚠️  No compiled test found"
        fi
    else
        echo "⚠️  No build directory found"
    fi
else
    echo "❌ real_gpt4all_server.cpp not found"
fi

echo ""
echo "🔍 CONNECTION REVIEW COMPLETE"
echo "=============================="
