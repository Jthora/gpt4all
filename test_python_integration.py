#!/usr/bin/env python3
"""
Comprehensive Python integration test for MinimalSimpleServer
Tests real-world usage scenarios that Python applications might use
"""

import requests
import json
import time
import threading
import concurrent.futures
from typing import Dict, Any
import sys

class GPT4AllAPITester:
    def __init__(self, base_url: str = "http://localhost:4891"):
        self.base_url = base_url
        self.session = requests.Session()
        self.tests_passed = 0
        self.tests_failed = 0
    
    def run_test(self, test_name: str, test_func, *args, **kwargs) -> bool:
        """Run a single test and track results"""
        print(f"Testing {test_name}... ", end="", flush=True)
        try:
            result = test_func(*args, **kwargs)
            if result:
                print("✅ PASS")
                self.tests_passed += 1
                return True
            else:
                print("❌ FAIL")
                self.tests_failed += 1
                return False
        except Exception as e:
            print(f"❌ FAIL - Exception: {e}")
            self.tests_failed += 1
            return False
    
    def test_health_endpoint(self) -> bool:
        """Test health endpoint functionality"""
        response = self.session.get(f"{self.base_url}/health")
        if response.status_code != 200:
            return False
        
        data = response.json()
        required_fields = ["status", "timestamp", "service"]
        return all(field in data for field in required_fields) and data["status"] == "ok"
    
    def test_models_endpoint(self) -> bool:
        """Test models endpoint functionality"""
        response = self.session.get(f"{self.base_url}/v1/models")
        if response.status_code != 200:
            return False
        
        data = response.json()
        if data.get("object") != "list" or "data" not in data:
            return False
        
        if not data["data"]:
            return False
        
        model = data["data"][0]
        required_fields = ["id", "object", "owned_by"]
        return all(field in model for field in required_fields)
    
    def test_error_handling(self) -> bool:
        """Test 404 error handling"""
        response = self.session.get(f"{self.base_url}/nonexistent")
        return response.status_code == 404
    
    def test_cors_headers(self) -> bool:
        """Test CORS header presence"""
        response = self.session.get(f"{self.base_url}/health")
        cors_headers = [
            "Access-Control-Allow-Origin",
            "Access-Control-Allow-Methods"
        ]
        return all(header in response.headers for header in cors_headers)
    
    def test_options_method(self) -> bool:
        """Test OPTIONS method for CORS preflight"""
        response = self.session.options(f"{self.base_url}/health")
        return response.status_code == 200
    
    def test_concurrent_requests(self, num_requests: int = 20) -> bool:
        """Test concurrent request handling"""
        def make_request():
            response = self.session.get(f"{self.base_url}/health")
            return response.status_code == 200
        
        with concurrent.futures.ThreadPoolExecutor(max_workers=10) as executor:
            futures = [executor.submit(make_request) for _ in range(num_requests)]
            results = [future.result() for future in concurrent.futures.as_completed(futures)]
        
        return all(results)
    
    def test_response_time(self, max_time_ms: int = 100) -> bool:
        """Test response time performance"""
        start_time = time.time()
        response = self.session.get(f"{self.base_url}/health")
        end_time = time.time()
        
        response_time_ms = (end_time - start_time) * 1000
        return response.status_code == 200 and response_time_ms < max_time_ms
    
    def test_json_content_type(self) -> bool:
        """Test proper JSON content type"""
        response = self.session.get(f"{self.base_url}/health")
        content_type = response.headers.get("Content-Type", "")
        return "application/json" in content_type
    
    def test_persistent_connection(self) -> bool:
        """Test multiple requests on same session"""
        responses = []
        for _ in range(5):
            response = self.session.get(f"{self.base_url}/health")
            responses.append(response.status_code == 200)
            time.sleep(0.1)
        
        return all(responses)
    
    def test_large_concurrent_load(self) -> bool:
        """Test handling of large concurrent load"""
        def make_requests_batch():
            success_count = 0
            for _ in range(10):
                try:
                    response = self.session.get(f"{self.base_url}/health", timeout=5)
                    if response.status_code == 200:
                        success_count += 1
                except:
                    pass
            return success_count
        
        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
            futures = [executor.submit(make_requests_batch) for _ in range(5)]
            results = [future.result() for future in concurrent.futures.as_completed(futures)]
        
        # Expect at least 80% success rate under load
        total_requests = sum(results)
        return total_requests >= 40  # 80% of 50 requests
    
    def test_openai_compatibility(self) -> bool:
        """Test OpenAI API compatibility"""
        # Test models endpoint structure
        response = self.session.get(f"{self.base_url}/v1/models")
        if response.status_code != 200:
            return False
        
        data = response.json()
        
        # Check OpenAI-compatible structure
        if data.get("object") != "list":
            return False
        
        if "data" not in data or not isinstance(data["data"], list):
            return False
        
        # Check model object structure
        for model in data["data"]:
            required_fields = ["id", "object", "created", "owned_by"]
            if not all(field in model for field in required_fields):
                return False
            if model["object"] != "model":
                return False
        
        return True
    
    def run_all_tests(self):
        """Run comprehensive test suite"""
        print("🧪 Starting Python Integration Tests for MinimalSimpleServer")
        print("=" * 60)
        
        # Wait for server to be ready
        print("Waiting for server to be ready...")
        max_wait = 10
        for _ in range(max_wait):
            try:
                response = self.session.get(f"{self.base_url}/health", timeout=1)
                if response.status_code == 200:
                    break
            except:
                pass
            time.sleep(1)
        else:
            print("❌ Server not responding, aborting tests")
            return False
        
        print("✅ Server is ready!\n")
        
        # Run all tests
        self.run_test("Health endpoint functionality", self.test_health_endpoint)
        self.run_test("Models endpoint functionality", self.test_models_endpoint)
        self.run_test("404 error handling", self.test_error_handling)
        self.run_test("CORS headers present", self.test_cors_headers)
        self.run_test("OPTIONS method support", self.test_options_method)
        self.run_test("JSON content type", self.test_json_content_type)
        self.run_test("Response time < 100ms", self.test_response_time)
        self.run_test("Concurrent requests (20)", self.test_concurrent_requests)
        self.run_test("Persistent connection handling", self.test_persistent_connection)
        self.run_test("Large concurrent load", self.test_large_concurrent_load)
        self.run_test("OpenAI API compatibility", self.test_openai_compatibility)
        
        # Results
        print("\n" + "=" * 60)
        print("📊 Test Results:")
        print(f"✅ Passed: {self.tests_passed}")
        print(f"❌ Failed: {self.tests_failed}")
        print(f"📈 Success Rate: {self.tests_passed/(self.tests_passed + self.tests_failed)*100:.1f}%")
        
        if self.tests_failed == 0:
            print("\n🎉 ALL PYTHON INTEGRATION TESTS PASSED!")
            print("✅ MinimalSimpleServer is fully compatible with Python applications!")
            return True
        else:
            print(f"\n⚠️  {self.tests_failed} tests failed. Please review the issues.")
            return False

if __name__ == "__main__":
    tester = GPT4AllAPITester()
    success = tester.run_all_tests()
    sys.exit(0 if success else 1)
