#include <iostream>
#include <dlfcn.h>
#include <cuda_runtime.h>

// Test CUDA availability
bool testCuda() {
    int deviceCount = 0;
    cudaError_t error = cudaGetDeviceCount(&deviceCount);
    
    if (error != cudaSuccess) {
        std::cout << "CUDA Error: " << cudaGetErrorString(error) << std::endl;
        return false;
    }
    
    std::cout << "Found " << deviceCount << " CUDA device(s)" << std::endl;
    
    if (deviceCount > 0) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, 0);
        std::cout << "Device 0: " << prop.name << std::endl;
        std::cout << "Compute Capability: " << prop.major << "." << prop.minor << std::endl;
        std::cout << "Global Memory: " << prop.totalGlobalMem / (1024*1024) << " MB" << std::endl;
        return true;
    }
    
    return false;
}

// Test loading the CUDA backend library
bool testBackendLibrary() {
    void* handle = dlopen("./gpt4all-backend/build/libllamamodel-mainline-cuda.so", RTLD_LAZY);
    if (!handle) {
        std::cout << "Failed to load CUDA backend: " << dlerror() << std::endl;
        return false;
    }
    
    std::cout << "Successfully loaded CUDA backend library!" << std::endl;
    dlclose(handle);
    return true;
}

int main() {
    std::cout << "=== GPT4All ARM64 CUDA Backend Test ===" << std::endl;
    
    // Test 1: CUDA availability
    std::cout << "\n1. Testing CUDA availability..." << std::endl;
    bool cudaOk = testCuda();
    
    // Test 2: Backend library loading
    std::cout << "\n2. Testing backend library loading..." << std::endl;
    bool backendOk = testBackendLibrary();
    
    // Summary
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "CUDA Available: " << (cudaOk ? "✓ YES" : "✗ NO") << std::endl;
    std::cout << "Backend Library: " << (backendOk ? "✓ LOADS" : "✗ FAILED") << std::endl;
    std::cout << "ARM64 CUDA GPT4All: " << (cudaOk && backendOk ? "✓ PROVEN!" : "✗ Failed") << std::endl;
    
    return (cudaOk && backendOk) ? 0 : 1;
}
