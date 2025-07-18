#!/usr/bin/env python3
"""
GPT4All Local API Server Test Script

This script tests the local API server functionality:
1. Checks if the server is running on the configured port
2. Tests OpenAI-compatible endpoints
3. Provides detailed feedback on server status
"""

import requests
import json
import time
import socket
import sys
from typing import Dict, Any

def check_port_open(host: str = "localhost", port: int = 4891) -> bool:
    """Check if a port is open and listening"""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(1)
            result = sock.connect_ex((host, port))
            return result == 0
    except Exception as e:
        print(f"Error checking port: {e}")
        return False

def test_api_endpoint(base_url: str = "http://localhost:4891") -> Dict[str, Any]:
    """Test the OpenAI-compatible API endpoints"""
    results = {}
    
    # Test health/info endpoint
    try:
        response = requests.get(f"{base_url}/", timeout=5)
        results["root_endpoint"] = {
            "status_code": response.status_code,
            "content": response.text[:200]
        }
    except Exception as e:
        results["root_endpoint"] = {"error": str(e)}
    
    # Test models endpoint
    try:
        response = requests.get(f"{base_url}/v1/models", timeout=5)
        results["models_endpoint"] = {
            "status_code": response.status_code,
            "content": response.text[:200]
        }
    except Exception as e:
        results["models_endpoint"] = {"error": str(e)}
    
    # Test chat completions endpoint
    try:
        test_payload = {
            "model": "gpt-3.5-turbo",
            "messages": [
                {"role": "user", "content": "Hello, this is a test message"}
            ],
            "stream": False
        }
        response = requests.post(
            f"{base_url}/v1/chat/completions",
            json=test_payload,
            timeout=10
        )
        results["chat_endpoint"] = {
            "status_code": response.status_code,
            "content": response.text[:200]
        }
    except Exception as e:
        results["chat_endpoint"] = {"error": str(e)}
    
    return results

def main():
    print("🔍 GPT4All Local API Server Test")
    print("=" * 50)
    
    port = 4891
    
    # Check if port is open
    print(f"📡 Checking if port {port} is open...")
    if check_port_open("localhost", port):
        print(f"✅ Port {port} is OPEN and listening")
        
        print(f"\n🧪 Testing API endpoints...")
        results = test_api_endpoint(f"http://localhost:{port}")
        
        for endpoint, result in results.items():
            print(f"\n📋 {endpoint}:")
            if "error" in result:
                print(f"  ❌ Error: {result['error']}")
            else:
                print(f"  📊 Status Code: {result['status_code']}")
                print(f"  📄 Response: {result['content']}")
        
        # Test with curl command suggestion
        print(f"\n💡 Manual test with curl:")
        print(f"curl -X POST http://localhost:{port}/v1/chat/completions \\")
        print('  -H "Content-Type: application/json" \\')
        print('  -d \'{"model":"gpt-3.5-turbo","messages":[{"role":"user","content":"Hello"}]}\'')
        
    else:
        print(f"❌ Port {port} is CLOSED or not listening")
        print(f"\n🔧 This means the local API server is NOT running")
        print(f"📋 Possible reasons:")
        print(f"  • Server not enabled in GPT4All settings")
        print(f"  • Qt 6.2 compatibility issues (QHttpServer unavailable)")
        print(f"  • GPT4All not running")
        print(f"  • Different port configured")
        
        print(f"\n💡 To enable (if supported):")
        print(f"  1. Open GPT4All")
        print(f"  2. Go to Settings → Application Settings")
        print(f"  3. Look for 'Enable Local API Server'")
        print(f"  4. Set port to {port}")
        print(f"  5. Restart GPT4All")

if __name__ == "__main__":
    main()
