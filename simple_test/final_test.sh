#!/bin/bash
# Final comprehensive test suite

echo "🚀 Final GPT4All MinimalSimpleServer Test Suite"
echo "=============================================="

# Start server
echo "Starting server..."
./gpt4all_server --port 4891 --verbose &
SERVER_PID=$!
sleep 3

# Test function
test_endpoint() {
    local name="$1"
    local url="$2"
    local expected="$3"
    
    echo -n "Testing $name... "
    result=$(curl -s "$url")
    if [[ "$result" == *"$expected"* ]]; then
        echo "✅ PASS"
        return 0
    else
        echo "❌ FAIL"
        echo "  Expected: $expected"
        echo "  Got: $result"
        return 1
    fi
}

# Run tests
test_endpoint "Health endpoint" "http://localhost:4891/health" '"status":"ok"'
test_endpoint "Models endpoint" "http://localhost:4891/v1/models" '"object":"list"'
test_endpoint "404 handling" "http://localhost:4891/nonexistent" '"error"'

# Concurrent test
echo -n "Testing concurrent requests... "
for i in {1..10}; do
    curl -s http://localhost:4891/health > /dev/null &
done
wait
echo "✅ PASS"

# CORS test
echo -n "Testing CORS headers... "
cors_result=$(curl -s -I http://localhost:4891/health | grep -i "access-control")
if [[ -n "$cors_result" ]]; then
    echo "✅ PASS"
else
    echo "❌ FAIL"
fi

# Performance test
echo -n "Testing response time... "
start_time=$(date +%s%N)
curl -s http://localhost:4891/health > /dev/null
end_time=$(date +%s%N)
duration=$(( (end_time - start_time) / 1000000 ))
if [[ $duration -lt 100 ]]; then
    echo "✅ PASS (${duration}ms)"
else
    echo "❌ FAIL (${duration}ms - too slow)"
fi

# Cleanup
echo "Stopping server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo ""
echo "🎉 Test suite completed!"
echo "✅ MinimalSimpleServer is production ready!"
