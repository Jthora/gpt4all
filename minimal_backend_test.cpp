#include <iostream>
#include <string>
#include <filesystem>
#include <vector>

// GPT4All backend C API
#include "gpt4all-backend/include/gpt4all-backend/llmodel_c.h"

int main() {
    std::cout << "🔧 Testing GPT4All Backend Discovery..." << std::endl;
    
    // Test backend path discovery
    std::vector<std::string> backend_paths = {
        "./build/bin",                    
        "./gpt4all-backend/build",        
        ".",                              
    };
    
    std::string active_backend_path = "."; 
    for (const auto& path : backend_paths) {
        std::string cpu_backend = path + "/libllamamodel-mainline-cpu.so";
        if (std::filesystem::exists(cpu_backend)) {
            active_backend_path = path;
            std::cout << "✅ Found backend libraries in: " << path << std::endl;
            break;
        }
    }
    
    // Set the implementation search path using GPT4All C API
    std::cout << "🎯 Setting GPT4All backend search path to: " << active_backend_path << std::endl;
    llmodel_set_implementation_search_path(active_backend_path.c_str());
    
    // Verify the setting worked
    const char* current_search_path = llmodel_get_implementation_search_path();
    std::cout << "🔍 GPT4All implementation search path confirmed: " << current_search_path << std::endl;
    
    // Test model creation
    std::cout << "\n🤖 Testing model creation..." << std::endl;
    
    // Find a model to test with
    std::string test_model_path;
    std::vector<std::string> model_paths = {
        "/media/jono/nvme/models/gpt4all",
        "."
    };
    
    for (const auto& model_dir : model_paths) {
        if (std::filesystem::exists(model_dir) && std::filesystem::is_directory(model_dir)) {
            for (const auto& entry : std::filesystem::directory_iterator(model_dir)) {
                if (entry.is_regular_file()) {
                    auto ext = entry.path().extension().string();
                    if (ext == ".gguf" || ext == ".bin") {
                        test_model_path = entry.path().string();
                        std::cout << "📁 Found test model: " << test_model_path << std::endl;
                        break;
                    }
                }
            }
            if (!test_model_path.empty()) break;
        }
    }
    
    if (test_model_path.empty()) {
        std::cout << "❌ No models found for testing" << std::endl;
        return 1;
    }
    
    // Test model creation
    const char* error = nullptr;
    llmodel_model model = llmodel_model_create2(test_model_path.c_str(), "cpu", &error);
    
    if (model) {
        std::cout << "✅ Model created successfully!" << std::endl;
        
        // Test model loading
        if (llmodel_loadModel(model, test_model_path.c_str(), 2048, -1)) {
            std::cout << "✅ Model loaded successfully!" << std::endl;
            
            if (llmodel_isModelLoaded(model)) {
                std::cout << "✅ Model is properly loaded and ready for inference!" << std::endl;
                std::cout << "🚀 Backend discovery and model loading WORKING!" << std::endl;
            } else {
                std::cout << "❌ Model not properly loaded" << std::endl;
            }
        } else {
            std::cout << "❌ Failed to load model weights" << std::endl;
        }
        
        llmodel_model_destroy(model);
    } else {
        std::cout << "❌ Failed to create model: " << (error ? error : "unknown error") << std::endl;
        return 1;
    }
    
    return 0;
}
