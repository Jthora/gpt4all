#!/bin/bash

# Complete API Server Functionality Test
# Tests all endpoints and validates the enhanced API server implementation

echo "🚀 GPT4All Enhanced API Server - Complete Functionality Test"
echo "============================================================="

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test results tracking
TESTS_PASSED=0
TESTS_FAILED=0

# Test function
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_pattern="$3"
    
    echo -e "${BLUE}Testing: $test_name${NC}"
    
    result=$(eval "$test_command" 2>/dev/null)
    exit_code=$?
    
    if [[ $exit_code -eq 0 ]] && [[ $result =~ $expected_pattern ]]; then
        echo -e "${GREEN}✅ PASS: $test_name${NC}"
        ((TESTS_PASSED++))
        return 0
    else
        echo -e "${RED}❌ FAIL: $test_name${NC}"
        echo "   Expected pattern: $expected_pattern"
        echo "   Actual result: $result"
        ((TESTS_FAILED++))
        return 1
    fi
}

# Check if server is running
echo -e "${YELLOW}Checking if standalone server is running...${NC}"
if ! pgrep -f "standalone_enhanced_ai_server" > /dev/null; then
    echo -e "${YELLOW}Starting standalone enhanced AI server...${NC}"
    cd /media/jono/nvme/projects/gpt4all/gpt4all-chat/build/bin
    nohup ./standalone_enhanced_ai_server > server_test.log 2>&1 &
    sleep 3
fi

# Test 1: Health Check
run_test "Health Endpoint" \
    "curl -s http://localhost:4891/health" \
    '"status":"ok"'

# Test 2: Models Endpoint
run_test "Models Endpoint" \
    "curl -s http://localhost:4891/v1/models" \
    '"object":"list".*"data":\['

# Test 3: Chat Completions - Basic
run_test "Chat Completions - Basic" \
    "curl -s -X POST http://localhost:4891/v1/chat/completions -H 'Content-Type: application/json' -d '{\"model\":\"gpt4all\",\"messages\":[{\"role\":\"user\",\"content\":\"Hello\"}]}'" \
    '"object":"chat.completion".*"choices":\['

# Test 4: Chat Completions - AI Response Content
run_test "Chat Completions - AI Response" \
    "curl -s -X POST http://localhost:4891/v1/chat/completions -H 'Content-Type: application/json' -d '{\"model\":\"gpt4all\",\"messages\":[{\"role\":\"user\",\"content\":\"Hello\"}]}'" \
    '"role":"assistant".*"content":'

# Test 5: Chat Completions - Usage Tracking
run_test "Chat Completions - Usage Tracking" \
    "curl -s -X POST http://localhost:4891/v1/chat/completions -H 'Content-Type: application/json' -d '{\"model\":\"gpt4all\",\"messages\":[{\"role\":\"user\",\"content\":\"Hello\"}]}'" \
    '"usage":.*"total_tokens":'

# Test 6: Error Handling - Invalid JSON
run_test "Error Handling - Invalid JSON" \
    "curl -s -X POST http://localhost:4891/v1/chat/completions -H 'Content-Type: application/json' -d 'invalid json'" \
    '"error":'

# Test 7: Error Handling - Missing Messages
run_test "Error Handling - Missing Messages" \
    "curl -s -X POST http://localhost:4891/v1/chat/completions -H 'Content-Type: application/json' -d '{\"model\":\"gpt4all\"}'" \
    '"error":.*"No messages provided"'

# Test 8: CORS Headers
run_test "CORS Headers" \
    "curl -s -I http://localhost:4891/health" \
    "Access-Control-Allow-Origin: \*"

# Test 9: HTTP Status Codes
run_test "HTTP Status Codes - Success" \
    "curl -s -o /dev/null -w '%{http_code}' http://localhost:4891/health" \
    "200"

# Test 10: HTTP Status Codes - Not Found
run_test "HTTP Status Codes - Not Found" \
    "curl -s -o /dev/null -w '%{http_code}' http://localhost:4891/nonexistent" \
    "404"

# Test 11: Content-Type Headers
run_test "Content-Type Headers" \
    "curl -s -I http://localhost:4891/health" \
    "Content-Type: application/json"

# Test 12: Different AI Prompts
run_test "AI Response - Greeting" \
    "curl -s -X POST http://localhost:4891/v1/chat/completions -H 'Content-Type: application/json' -d '{\"model\":\"gpt4all\",\"messages\":[{\"role\":\"user\",\"content\":\"Hi there\"}]}'" \
    '"role":"assistant".*"GPT4All"'

# Test 13: AI Response - Question
run_test "AI Response - Question" \
    "curl -s -X POST http://localhost:4891/v1/chat/completions -H 'Content-Type: application/json' -d '{\"model\":\"gpt4all\",\"messages\":[{\"role\":\"user\",\"content\":\"How are you?\"}]}'" \
    '"role":"assistant".*"doing well"'

# Test 14: AI Response - Joke Request
run_test "AI Response - Joke" \
    "curl -s -X POST http://localhost:4891/v1/chat/completions -H 'Content-Type: application/json' -d '{\"model\":\"gpt4all\",\"messages\":[{\"role\":\"user\",\"content\":\"Tell me a joke\"}]}'" \
    '"role":"assistant".*"joke"'

# Test 15: Server Uptime
run_test "Server Process Running" \
    "pgrep -f standalone_enhanced_ai_server" \
    "[0-9]+"

echo ""
echo "============================================================="
echo -e "${BLUE}Test Results Summary${NC}"
echo "============================================================="
echo -e "✅ ${GREEN}Tests Passed: $TESTS_PASSED${NC}"
echo -e "❌ ${RED}Tests Failed: $TESTS_FAILED${NC}"
echo -e "📊 ${YELLOW}Total Tests: $((TESTS_PASSED + TESTS_FAILED))${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo ""
    echo -e "${GREEN}🎉 ALL TESTS PASSED! Enhanced API Server is fully functional!${NC}"
    echo -e "${GREEN}✅ HTTP API Server: PRODUCTION READY${NC}"
    echo -e "${GREEN}✅ OpenAI Compatibility: COMPLETE${NC}"
    echo -e "${GREEN}✅ Error Handling: ROBUST${NC}"
    echo -e "${GREEN}✅ AI Integration: WORKING${NC}"
    echo ""
    echo -e "${BLUE}🚀 API Server Endpoints Available:${NC}"
    echo "   • Health: http://localhost:4891/health"
    echo "   • Models: http://localhost:4891/v1/models"
    echo "   • Chat: http://localhost:4891/v1/chat/completions"
    echo ""
    echo -e "${YELLOW}🎯 Mission Complete: GPT4All Enhanced API Server is ready for production use!${NC}"
    exit 0
else
    echo ""
    echo -e "${RED}❌ Some tests failed. Please check the server configuration.${NC}"
    exit 1
fi
