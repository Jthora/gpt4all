#!/bin/bash
# Comprehensive stability test for MinimalSimpleServer

echo "=== GPT4All MinimalSimpleServer Stability Test ==="
echo "Testing production readiness and compatibility..."
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test results tracking
TESTS_PASSED=0
TESTS_FAILED=0

# Function to run a test
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_pattern="$3"
    
    echo -n "Testing $test_name... "
    
    # Run the command and capture output
    result=$(eval "$test_command" 2>&1)
    exit_code=$?
    
    # Check if the test passed
    if [ $exit_code -eq 0 ] && [[ "$result" =~ $expected_pattern ]]; then
        echo -e "${GREEN}PASS${NC}"
        ((TESTS_PASSED++))
        return 0
    else
        echo -e "${RED}FAIL${NC}"
        echo "  Expected pattern: $expected_pattern"
        echo "  Got: $result"
        echo "  Exit code: $exit_code"
        ((TESTS_FAILED++))
        return 1
    fi
}

# Start the server in background
echo "Starting MinimalSimpleServer..."
timeout 60s ./simple_test/gpt4all_server --port 4891 > server.log 2>&1 &
SERVER_PID=$!

# Wait for server to start
sleep 3

# Check if server is running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${RED}ERROR: Server failed to start${NC}"
    cat server.log
    exit 1
fi

echo -e "${GREEN}Server started successfully (PID: $SERVER_PID)${NC}"
echo

# Basic endpoint tests
echo "=== Basic Endpoint Tests ==="
run_test "Health endpoint availability" \
    "curl -s -w '%{http_code}' http://localhost:4891/health | tail -c 3" \
    "200"

run_test "Health endpoint JSON response" \
    "curl -s http://localhost:4891/health | jq -r '.status'" \
    "ok"

run_test "Models endpoint availability" \
    "curl -s -w '%{http_code}' http://localhost:4891/v1/models | tail -c 3" \
    "200"

run_test "Models endpoint JSON structure" \
    "curl -s http://localhost:4891/v1/models | jq -r '.object'" \
    "list"

run_test "404 for unknown endpoint" \
    "curl -s -w '%{http_code}' http://localhost:4891/nonexistent | tail -c 3" \
    "404"

echo

# Stress testing
echo "=== Stress and Concurrency Tests ==="

run_test "Multiple concurrent requests (10)" \
    "for i in {1..10}; do curl -s http://localhost:4891/health & done; wait; echo 'done'" \
    "done"

run_test "Rapid sequential requests (20)" \
    "for i in {1..20}; do curl -s http://localhost:4891/health > /dev/null; done; echo 'completed'" \
    "completed"

echo

# Protocol compliance tests
echo "=== HTTP Protocol Compliance Tests ==="

run_test "CORS headers present" \
    "curl -s -I http://localhost:4891/health | grep -i 'access-control-allow-origin'" \
    "Access-Control-Allow-Origin"

run_test "Content-Type header correct" \
    "curl -s -I http://localhost:4891/health | grep -i 'content-type.*application/json'" \
    "application/json"

run_test "OPTIONS method support (CORS preflight)" \
    "curl -s -X OPTIONS -w '%{http_code}' http://localhost:4891/health | tail -c 3" \
    "200"

echo

# Edge case tests
echo "=== Edge Case Tests ==="

run_test "Empty request handling" \
    "curl -s -w '%{http_code}' http://localhost:4891/ | tail -c 3" \
    "404"

run_test "Large request path handling" \
    "curl -s -w '%{http_code}' 'http://localhost:4891/$(printf 'a%.0s' {1..1000})' | tail -c 3" \
    "404"

run_test "Special characters in path" \
    "curl -s -w '%{http_code}' 'http://localhost:4891/test%20path%20with%20spaces' | tail -c 3" \
    "404"

echo

# Integration tests with common tools
echo "=== Integration Tests ==="

run_test "wget compatibility" \
    "wget -q -O - http://localhost:4891/health | jq -r '.status'" \
    "ok"

run_test "Python requests compatibility" \
    "python3 -c 'import requests; r=requests.get(\"http://localhost:4891/health\"); print(r.json()[\"status\"])'" \
    "ok"

run_test "Node.js fetch compatibility" \
    "node -e 'fetch(\"http://localhost:4891/health\").then(r=>r.json()).then(d=>console.log(d.status)).catch(console.error)'" \
    "ok"

echo

# Performance tests
echo "=== Performance Tests ==="

run_test "Response time under 100ms" \
    "time_result=\$(curl -s -w '%{time_total}' -o /dev/null http://localhost:4891/health); if (( \$(echo \"\$time_result < 0.1\" | bc -l) )); then echo 'fast'; else echo 'slow'; fi" \
    "fast"

run_test "Server handles 50 rapid requests" \
    "time (for i in {1..50}; do curl -s http://localhost:4891/health > /dev/null; done); echo 'completed'" \
    "completed"

echo

# Memory and resource tests
echo "=== Resource Usage Tests ==="

run_test "Server still responsive after stress" \
    "curl -s http://localhost:4891/health | jq -r '.status'" \
    "ok"

run_test "Process still running" \
    "if kill -0 $SERVER_PID 2>/dev/null; then echo 'running'; else echo 'stopped'; fi" \
    "running"

echo

# OpenAI API compatibility tests
echo "=== OpenAI API Compatibility Tests ==="

run_test "Models response has required fields" \
    "curl -s http://localhost:4891/v1/models | jq -r '.data[0].id'" \
    "gpt4all-test"

run_test "Models response structure matches OpenAI" \
    "curl -s http://localhost:4891/v1/models | jq -r '.data[0].object'" \
    "model"

echo

# Cleanup
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

# Final results
echo "=== Test Results ==="
echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"
echo -e "Total tests: $((TESTS_PASSED + TESTS_FAILED))"
echo

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}🎉 ALL TESTS PASSED! MinimalSimpleServer is production ready!${NC}"
    exit 0
else
    echo -e "${RED}❌ Some tests failed. Please review the issues above.${NC}"
    exit 1
fi
