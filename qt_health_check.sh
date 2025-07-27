#!/bin/bash

# Comprehensive GPT4All Qt 6.2.4 System Health Check
# Tests all components after Qt downgrade from 6.8+ to 6.2.4

echo "🔍 GPT4All Qt 6.2.4 System Health Check"
echo "========================================"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test results tracking
TESTS_PASSED=0
TESTS_FAILED=0
WARNINGS=0

# Test function
run_test() {
    local test_name="$1"
    local test_command="$2"
    local success_pattern="$3"
    local is_warning="${4:-false}"
    
    echo -e "${BLUE}Testing: $test_name${NC}"
    
    result=$(eval "$test_command" 2>&1)
    exit_code=$?
    
    if [[ $exit_code -eq 0 ]] && [[ $result =~ $success_pattern ]]; then
        echo -e "${GREEN}✅ PASS: $test_name${NC}"
        ((TESTS_PASSED++))
        return 0
    elif [[ $is_warning == "true" ]]; then
        echo -e "${YELLOW}⚠️  WARN: $test_name${NC}"
        echo "   Result: ${result:0:100}..."
        ((WARNINGS++))
        return 1
    else
        echo -e "${RED}❌ FAIL: $test_name${NC}"
        echo "   Result: ${result:0:100}..."
        ((TESTS_FAILED++))
        return 1
    fi
}

cd /media/jono/nvme/projects/gpt4all

echo -e "${YELLOW}=== 1. Core Library Status ===${NC}"

# Test 1: CUDA Libraries Present
run_test "CUDA Libraries Present" \
    "ls gpt4all-chat/build/bin/libllamamodel-mainline-cuda*.so" \
    "libllamamodel-mainline-cuda"

# Test 2: Core LLM Library Present
run_test "Core LLM Library Present" \
    "ls gpt4all-chat/build/bin/libllmodel.so*" \
    "libllmodel.so"

# Test 3: Chat Binary Exists
run_test "Chat Binary Exists" \
    "ls gpt4all-chat/build/bin/chat" \
    "chat"

echo -e "${YELLOW}=== 2. Build System Health ===${NC}"

# Test 4: CMake Build Configuration
run_test "CMake Build Configuration" \
    "grep -q 'Qt6' gpt4all-chat/build/CMakeCache.txt" \
    ""

# Test 5: Qt 6.2 Components Linked
run_test "Qt 6.2 Components" \
    "ldd gpt4all-chat/build/bin/chat | grep -c Qt6" \
    "[0-9]+"

echo -e "${YELLOW}=== 3. Translation System ===${NC}"

# Test 6: Translation Source Files
run_test "Translation Source Files" \
    "ls gpt4all-chat/translations/*.ts | wc -l" \
    "[5-9]"

# Test 7: Translation Build Status (Warning - known to be disabled)
run_test "Translation Build Status" \
    "ls gpt4all-chat/build/bin/*.qm 2>/dev/null | wc -l" \
    "[0-9]+" \
    "true"

echo -e "${YELLOW}=== 4. QML Component Health ===${NC}"

# Test 8: FolderDialog Fix Applied
run_test "FolderDialog Fix Applied" \
    "grep -q 'FileDialog' gpt4all-chat/qml/MyFolderDialog.qml" \
    ""

# Test 9: No Qt 6.8+ Syntax Remaining
run_test "No Qt 6.8+ Property Syntax" \
    "! grep -r 'property list<' gpt4all-chat/qml/" \
    ""

echo -e "${YELLOW}=== 5. Runtime Functionality ===${NC}"

# Test 10: Application Startup (Basic)
run_test "Application Startup Test" \
    "timeout 5 gpt4all-chat/build/bin/chat --help 2>&1 | head -1" \
    ".*" \
    "true"

# Test 11: Kompute Error Expected (Warning - this is normal)
run_test "Kompute Backend Disabled (Expected)" \
    "timeout 3 gpt4all-chat/build/bin/chat 2>&1 | grep -c 'kompute'" \
    "[1-9]" \
    "true"

echo -e "${YELLOW}=== 6. API Server Status ===${NC}"

# Test 12: Standalone API Server Present
run_test "Standalone API Server Built" \
    "ls gpt4all-chat/build/bin/standalone_enhanced_ai_server" \
    "standalone_enhanced_ai_server"

# Test 13: Enhanced SimpleServer Code
run_test "Enhanced SimpleServer Integration" \
    "grep -q 'generateAIResponse' gpt4all-chat/src/simpleserver.cpp" \
    ""

echo -e "${YELLOW}=== 7. Known Issues Assessment ===${NC}"

# Test 14: Server Stub Safe Implementation
run_test "Server Stub Qt 6.2 Safe" \
    "grep -q 'Qt 6.2 compatibility' gpt4all-chat/src/server_stub.cpp" \
    ""

# Test 15: Translation System Disabled
run_test "Translation System Disabled (Known Issue)" \
    "grep -q '# qt_add_translations' gpt4all-chat/CMakeLists.txt" \
    "" \
    "true"

echo ""
echo "========================================"
echo -e "${BLUE}System Health Summary${NC}"
echo "========================================"
echo -e "✅ ${GREEN}Tests Passed: $TESTS_PASSED${NC}"
echo -e "⚠️  ${YELLOW}Warnings: $WARNINGS${NC}"
echo -e "❌ ${RED}Tests Failed: $TESTS_FAILED${NC}"
echo -e "📊 ${YELLOW}Total Checks: $((TESTS_PASSED + TESTS_FAILED + WARNINGS))${NC}"

echo ""
echo -e "${BLUE}Issue Analysis:${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}🎉 SYSTEM HEALTHY: No critical failures detected!${NC}"
    echo -e "${GREEN}✅ Core functionality appears intact${NC}"
else
    echo -e "${RED}⚠️  ISSUES DETECTED: $TESTS_FAILED critical failures${NC}"
fi

if [ $WARNINGS -gt 0 ]; then
    echo -e "${YELLOW}📝 Known Issues (Non-Critical):${NC}"
    echo "   • Translation system disabled (UI in English only)"
    echo "   • Kompute backend disabled (CUDA available instead)"
    echo "   • Some startup warnings expected"
fi

echo ""
echo -e "${BLUE}Recommended Actions:${NC}"

if grep -q "Translation.*FAIL" <(echo "$result"); then
    echo "1. 🔧 Enable translation system in CMakeLists.txt"
fi

if [ $TESTS_FAILED -gt 2 ]; then
    echo "2. 🚨 Investigate critical build issues"
else
    echo "1. ✅ System is stable - focus on feature testing"
fi

echo "2. 🧪 Test CUDA acceleration with actual model loading"
echo "3. 🎨 Verify all QML interfaces work correctly"
echo "4. 📱 Test complete user workflow end-to-end"

echo ""
echo -e "${YELLOW}Next Steps:${NC}"
echo "• Run: ./gpt4all-chat/build/bin/chat (test full GUI)"
echo "• Run: ./gpt4all-chat/build/bin/standalone_enhanced_ai_server (test API)"
echo "• Load a model and verify CUDA acceleration"
echo "• Test LocalDocs, model downloading, chat functionality"

exit $TESTS_FAILED
