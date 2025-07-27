#!/usr/bin/env python3
"""
Simple GPT4All Runner
Launches GPT4All with CUDA support to test the system
"""
import os
import sys

def main():
    """Run GPT4All application"""
    print("🚀 GPT4All Application Launcher")
    print("=" * 50)
    
    # Set library path for our compiled libraries
    backend_path = "/media/jono/nvme/projects/gpt4all/gpt4all-backend/build"
    current_ld_path = os.environ.get('LD_LIBRARY_PATH', '')
    os.environ['LD_LIBRARY_PATH'] = f"{backend_path}:{current_ld_path}"
    
    print(f"📚 Library path set: {backend_path}")
    
    try:
        print("🔄 Importing GPT4All...")
        import gpt4all
        print("✅ GPT4All imported successfully")
        
        print("\n📋 Available models:")
        models = gpt4all.GPT4All.list_models()
        
        if not models:
            print("  ⚠️  No models found. Let's download a small one...")
            print("  💡 This will download orca-mini-3b (~2GB)")
            
            try:
                # Download a small model for testing
                model = gpt4all.GPT4All("orca-mini-3b-gguf2-q4_0.gguf")
                print("✅ Model downloaded successfully!")
                
                print("\n🎮 Testing CUDA acceleration...")
                print("Ask a question (or 'quit' to exit):")
                
                while True:
                    question = input("\n> ")
                    if question.lower() in ['quit', 'exit', 'q']:
                        break
                    
                    print("🤔 Thinking...")
                    response = model.generate(question, max_tokens=100)
                    print(f"🤖 GPT4All: {response}")
                
            except Exception as e:
                print(f"❌ Model download/usage failed: {e}")
                print("💡 You may need to download a model manually")
                
        else:
            print("✅ Found existing models:")
            for model in models[:5]:  # Show first 5
                print(f"  📄 {model.get('filename', 'Unknown')}")
            
            # Try to use the first available model
            try:
                first_model = models[0].get('filename')
                if first_model:
                    print(f"\n🔄 Loading model: {first_model}")
                    model = gpt4all.GPT4All(first_model)
                    
                    print("✅ Model loaded! Ready for chat.")
                    print("Ask a question (or 'quit' to exit):")
                    
                    while True:
                        question = input("\n> ")
                        if question.lower() in ['quit', 'exit', 'q']:
                            break
                        
                        print("🤔 Thinking...")
                        response = model.generate(question, max_tokens=100)
                        print(f"🤖 GPT4All: {response}")
            except Exception as e:
                print(f"❌ Model loading failed: {e}")
        
    except ImportError:
        print("❌ GPT4All Python package not available")
        print("💡 Try: pip3 install gpt4all")
        return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    
    print("\n👋 GPT4All session ended. Thanks for testing!")
    return True

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
