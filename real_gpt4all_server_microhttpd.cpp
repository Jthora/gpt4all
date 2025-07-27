#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <memory>
#include <microhttpd.h>
#include <string>
#include <cstring>
#include <csignal>
#include <atomic>

// Fix fmt compatibility issue - GPT4All expects fmt/base.h but new fmt uses core.h
#ifndef FMT_BASE_H_
#define FMT_BASE_H_
// We'll use Qt strings instead of fmt to avoid compatibility issues
#endif

// REAL GPT4ALL BACKEND INTEGRATION - C API
#include "gpt4all-backend/include/gpt4all-backend/llmodel_c.h"

// Use clean backend instead - no fmt/nlohmann dependencies
#include <iostream>
#include <filesystem>
#include <dlfcn.h>
#include <unordered_map>
#include <chrono>

// REAL AI INFERENCE CALLBACKS FOR C API
static QString* g_ai_accumulator = nullptr;

static bool ai_prompt_callback(const token_t *token_ids, size_t n_token_ids, bool cached) {
    (void)token_ids; (void)n_token_ids; (void)cached; // Suppress warnings
    return true; // Continue processing
}

static bool ai_response_callback(token_t token_id, const char *response) {
    if (token_id == -1) {
        // Error token
        qWarning() << "❌ AI generation error:" << response;
        return false;
    }
    if (response && strlen(response) > 0 && g_ai_accumulator) {
        *g_ai_accumulator += QString::fromUtf8(response);
    }
    return true; // Continue generation
}

// AI Agent Persona Core Requirements
struct ConversationSession {
    std::string session_id;
    std::vector<std::pair<std::string, std::string>> history; // role, content pairs
    std::string system_prompt;
    std::string persona;
    std::chrono::system_clock::time_point last_activity;
    std::unordered_map<std::string, std::string> metadata;
};

struct ModelInfo {
    std::string path;
    std::string name;
    std::string type; // "chat", "code", "reasoning", etc.
    bool loaded = false;
    void* handle = nullptr;
};

struct GenerationParams {
    float temperature = 0.7f;
    int max_tokens = 1024;
    float top_p = 0.9f;
    float top_k = 40.0f;
    std::vector<std::string> stop_sequences;
    bool stream = false;
};

// Simplified model interface for direct backend usage
struct SimpleModel {
    void* handle = nullptr;
    std::string name;
    std::string path;
    bool loaded = false;
    
    // Function pointers for direct llmodel usage
    void* (*create_llmodel)(const char*) = nullptr;
    void (*delete_llmodel)(void*) = nullptr;
    bool (*load_model)(void*, const char*, int, int) = nullptr;
    std::string (*generate_response)(void*, const char*) = nullptr;
};

class GPT4AllAPIServer {

public:
    GPT4AllAPIServer() : m_daemon(nullptr) {
        initializeGPT4All();
    }
    
    ~GPT4AllAPIServer() {
        if (m_daemon) {
            MHD_stop_daemon(m_daemon);
        }
    }
    
    bool start(int port = 8080) {
        qDebug() << "🔄 Starting AI Agent server on port" << port;
        
        // Try simpler daemon configuration first
        m_daemon = MHD_start_daemon(
            MHD_USE_INTERNAL_POLLING_THREAD,
            port,
            nullptr, nullptr,
            &requestHandler, this,
            MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 120,
            MHD_OPTION_END
        );
        
        if (m_daemon) {
            qDebug() << "✅ AI Agent Persona Core Server listening on port" << port;
            return true;
        } else {
            qCritical() << "❌ Failed to start AI Agent server on port" << port;
            qCritical() << "   Error: Check if port is already in use or permission issues";
            return false;
        }
    }
    
    static void signalHandler(int signal) {
        qDebug() << "Received signal" << signal << "- shutting down gracefully...";
        s_shutdownRequested = true;
        QCoreApplication::quit();
    }
    
    static std::atomic<bool> s_shutdownRequested;

private:
    // REAL AI INITIALIZATION - Configure backend paths and discovery
    void initializeGPT4All() {
        qDebug() << "Initializing GPT4All backend for REAL AI inference...";
        
        // CRITICAL: Set backend implementation search path FIRST
        qDebug() << "🔧 Configuring GPT4All backend discovery...";
        
        // SOLUTION: Use GPT4All's setImplementationsSearchPath to configure backend discovery
        // This is the proper way to tell GPT4All where to find libllamamodel-mainline-cpu.so
        std::vector<std::string> backend_paths = {
            "./build/bin",                    // Primary build location
            "./gpt4all-backend/build",        // Alternative build location
            ".",                              // Current directory (has symlinks)
            "/usr/local/lib/gpt4all",         // System install location
            "/usr/lib/gpt4all"                // Alternative system location
        };
        
        // Find the first path that contains backend libraries
        std::string active_backend_path = "."; // Default fallback
        for (const auto& path : backend_paths) {
            std::string cpu_backend = path + "/libllamamodel-mainline-cpu.so";
            if (std::filesystem::exists(cpu_backend)) {
                active_backend_path = path;
                qDebug() << "✅ Found backend libraries in:" << QString::fromStdString(path);
                break;
            }
        }
        
        // Set the implementation search path using GPT4All C API
        qDebug() << "🎯 Setting GPT4All backend search path to:" << QString::fromStdString(active_backend_path);
        llmodel_set_implementation_search_path(active_backend_path.c_str());
        
        // Verify the setting worked
        const char* current_search_path = llmodel_get_implementation_search_path();
        qDebug() << "🔍 GPT4All implementation search path confirmed:" << current_search_path;
        
        // Also set LD_LIBRARY_PATH as backup
        const char* current_lib_path = getenv("LD_LIBRARY_PATH");
        std::string new_lib_path = active_backend_path;
        if (current_lib_path && strlen(current_lib_path) > 0) {
            new_lib_path += ":";
            new_lib_path += current_lib_path;
        }
        setenv("LD_LIBRARY_PATH", new_lib_path.c_str(), 1);
        
        qDebug() << "📚 LD_LIBRARY_PATH updated to:" << QString::fromStdString(new_lib_path);
        
        // Simple model discovery and loading
        tryLoadDefaultModel();
        
        qDebug() << "🚀 GPT4All backend initialization complete - READY FOR REAL AI!";
    }
    
    // ENHANCED MODEL LOADING FOR AI AGENT NEEDS
    void tryLoadDefaultModel() {
        qDebug() << "🔍 Searching for GPT4All models for AI Agent...";
        
        // Search for models in user's path with proper error handling
        std::vector<std::string> searchPaths = {
            "/media/jono/nvme/models/gpt4all",  // User's known path
            "/usr/share/gpt4all/models",
            "./models"
        };
        
        // Add HOME-based paths safely
        const char* home = getenv("HOME");
        if (home && strlen(home) > 0) {
            searchPaths.push_back(std::string(home) + "/.local/share/nomic.ai/GPT4All");
            searchPaths.push_back(std::string(home) + "/gpt4all_models");
        }
        
        m_availableModels.clear();
        for (const auto& searchPath : searchPaths) {
            try {
                if (std::filesystem::exists(searchPath) && std::filesystem::is_directory(searchPath)) {
                    for (const auto& entry : std::filesystem::directory_iterator(searchPath)) {
                        if (entry.is_regular_file()) {
                            auto ext = entry.path().extension().string();
                            // Validate file extensions and size
                            if ((ext == ".gguf" || ext == ".bin") && 
                                entry.file_size() > 1024*1024) { // At least 1MB
                                
                                ModelInfo model;
                                model.path = entry.path().string();
                                model.name = entry.path().stem().string();
                                
                                // Categorize models by name patterns for AI Agent
                                if (model.name.find("code") != std::string::npos || 
                                    model.name.find("coder") != std::string::npos) {
                                    model.type = "code";
                                } else if (model.name.find("instruct") != std::string::npos) {
                                    model.type = "chat";
                                } else if (model.name.find("math") != std::string::npos || 
                                          model.name.find("reasoning") != std::string::npos) {
                                    model.type = "reasoning";
                                } else {
                                    model.type = "general";
                                }
                                
                                m_availableModels.push_back(model);
                                qDebug() << "   Found" << QString::fromStdString(model.type) 
                                        << "model:" << QString::fromStdString(model.name);
                            }
                        }
                    }
                }
            } catch (const std::filesystem::filesystem_error& e) {
                qWarning() << "⚠️  Error accessing path" << QString::fromStdString(searchPath) 
                          << ":" << e.what();
            }
        }
        
        if (!m_availableModels.empty()) {
            // Default to first instruct model for AI Agent, fallback to first available
            m_currentModel = &m_availableModels[0];
            for (auto& model : m_availableModels) {
                if (model.type == "chat") {
                    m_currentModel = &model;
                    break;
                }
            }
            
            qDebug() << "✅ Selected model for AI Agent:" << QString::fromStdString(m_currentModel->name);
            qDebug() << "🤖 Model type:" << QString::fromStdString(m_currentModel->type);
            qDebug() << "📊 Available models:" << m_availableModels.size();
            qDebug() << "🔗 REAL AI inference ready for Persona Core!";
        } else {
            qDebug() << "❌ No GPT4All models found.";
            qDebug() << "     Please download a model from: https://gpt4all.io/models/";
        }
    }
    
    void setupServer() {
        // Use libmicrohttpd instead of QHttpServer with proper error handling
        m_daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                                    4891,
                                    nullptr, nullptr,
                                    &requestHandler, this,
                                    MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 120,
                                    MHD_OPTION_THREAD_POOL_SIZE, (unsigned int) 4,
                                    MHD_OPTION_END);
        
        if (m_daemon) {
            qDebug() << "✅ GPT4All API Server listening on port 4891 (libmicrohttpd)";
            qDebug() << "🌐 Available endpoints:";
            qDebug() << "   GET  http://localhost:4891/health";
            qDebug() << "   GET  http://localhost:4891/v1/models";
            qDebug() << "   POST http://localhost:4891/v1/chat/completions";
            qDebug() << "   POST http://localhost:4891/v1/sessions (AI Agent session management)";
            qDebug() << "   POST http://localhost:4891/v1/persona (Set AI Agent persona)";
            qDebug() << "🔧 Server features: timeout=120s, thread_pool=4, CORS enabled";
            qDebug() << "🤖 AI Agent features: sessions, personas, model selection";
        } else {
            qCritical() << "❌ Failed to start server on port 4891";
            qCritical() << "   Check if port is already in use: netstat -tlnp | grep 4891";
            qCritical() << "   Or try a different port";
            QCoreApplication::exit(1);
        }
    }
    
    // HTTP request handler using libmicrohttpd with proper memory management
    static enum MHD_Result requestHandler(void *cls,
                                          struct MHD_Connection *connection,
                                          const char *url,
                                          const char *method,
                                          const char *version,
                                          const char *upload_data,
                                          size_t *upload_data_size,
                                          void **con_cls) {
        
        GPT4AllAPIServer *server = static_cast<GPT4AllAPIServer*>(cls);
        
        // Proper POST body handling with memory safety
        struct RequestContext {
            std::string body;
            bool is_post;
        };
        
        RequestContext *ctx = static_cast<RequestContext*>(*con_cls);
        
        if (ctx == nullptr) {
            // First call for this request - allocate context
            ctx = new RequestContext();
            ctx->is_post = (strcmp(method, "POST") == 0);
            *con_cls = ctx;
            
            if (!ctx->is_post) {
                // For GET requests, we can process immediately
                return MHD_YES;
            }
            return MHD_YES;
        }
        
        // Handle POST data accumulation
        if (ctx->is_post && *upload_data_size > 0) {
            // Limit request body size to prevent DoS
            if (ctx->body.size() + *upload_data_size > 1024*1024) { // 1MB limit
                delete ctx;
                return MHD_NO;
            }
            ctx->body.append(upload_data, *upload_data_size);
            *upload_data_size = 0;
            return MHD_YES;
        }
        
        // Process the request
        std::string response;
        std::string content_type = "application/json";
        int status_code = MHD_HTTP_OK;
        
        try {
            if (strcmp(method, "GET") == 0 && strcmp(url, "/health") == 0) {
                response = server->handleHealth();
            } else if (strcmp(method, "GET") == 0 && strcmp(url, "/v1/models") == 0) {
                response = server->handleModels();
            } else if (strcmp(method, "POST") == 0 && strcmp(url, "/v1/chat/completions") == 0) {
                response = server->handleChatCompletion(ctx->body);
            } else if (strcmp(method, "POST") == 0 && strcmp(url, "/v1/sessions") == 0) {
                response = server->handleSessionManagement(ctx->body);
            } else if (strcmp(method, "POST") == 0 && strcmp(url, "/v1/persona") == 0) {
                response = server->handlePersonaUpdate(ctx->body);
            } else if (strcmp(method, "POST") == 0 && strcmp(url, "/v1/model/select") == 0) {
                response = server->handleModelSelection(ctx->body);
            } else {
                response = "{\"error\":\"Not found\"}";
                status_code = MHD_HTTP_NOT_FOUND;
            }
        } catch (const std::exception& e) {
            response = "{\"error\":\"Internal server error\"}";
            status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
            qWarning() << "Request handler exception:" << e.what();
        }
        
        // Clean up context
        delete ctx;
        *con_cls = nullptr;
        
        struct MHD_Response *mhd_response = MHD_create_response_from_buffer(
            response.length(),
            (void*)response.c_str(),
            MHD_RESPMEM_MUST_COPY);
        
        MHD_add_response_header(mhd_response, "Content-Type", content_type.c_str());
        MHD_add_response_header(mhd_response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(mhd_response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        MHD_add_response_header(mhd_response, "Access-Control-Allow-Headers", "Content-Type");
        
        enum MHD_Result ret = MHD_queue_response(connection, status_code, mhd_response);
        MHD_destroy_response(mhd_response);
        
        return ret;
    }
    
    std::string handleHealth() {
        QJsonObject response;
        response["status"] = "healthy";
        response["ai_agent_ready"] = true;
        response["models_available"] = static_cast<int>(m_availableModels.size());
        response["current_model"] = m_currentModel ? QString::fromStdString(m_currentModel->name) : "none";
        response["active_sessions"] = static_cast<int>(m_sessions.size());
        
        if (m_currentModel) {
            response["ai_inference"] = "REAL AI available for Persona Core";
            response["model_type"] = QString::fromStdString(m_currentModel->type);
            response["model_path"] = QString::fromStdString(m_currentModel->path);
        } else {
            response["ai_inference"] = "No model loaded";
        }
        
        // Add AI Agent capabilities
        QJsonArray capabilities;
        capabilities.append("session_management");
        capabilities.append("persona_injection");
        capabilities.append("model_selection");
        capabilities.append("conversation_history");
        capabilities.append("generation_parameters");
        response["ai_agent_capabilities"] = capabilities;
        
        QJsonDocument doc(response);
        return doc.toJson().toStdString();
    }
    
    std::string handleModels() {
        QJsonArray models;
        
        // List all available models with their capabilities
        for (const auto& model : m_availableModels) {
            QJsonObject modelObj;
            modelObj["id"] = QString::fromStdString(model.name);
            modelObj["object"] = "model";
            modelObj["type"] = QString::fromStdString(model.type);
            modelObj["path"] = QString::fromStdString(model.path);
            modelObj["loaded"] = model.loaded;
            modelObj["created"] = 1686935002;
            modelObj["owned_by"] = "nomic-ai";
            
            // Add AI Agent specific metadata
            QJsonArray capabilities;
            if (model.type == "code") {
                capabilities.append("programming");
                capabilities.append("debugging");
                capabilities.append("code_generation");
            } else if (model.type == "chat") {
                capabilities.append("conversation");
                capabilities.append("general_assistance");
                capabilities.append("persona_roleplay");
            } else if (model.type == "reasoning") {
                capabilities.append("mathematical_reasoning");
                capabilities.append("logical_analysis");
                capabilities.append("problem_solving");
            }
            modelObj["ai_agent_capabilities"] = capabilities;
            
            models.append(modelObj);
        }
        
        QJsonObject response;
        response["object"] = "list";
        response["data"] = models;
        response["current_model"] = m_currentModel ? QString::fromStdString(m_currentModel->name) : "none";
        
        QJsonDocument doc(response);
        return doc.toJson().toStdString();
    }
    
    std::string handleChatCompletion(const std::string &body) {
        if (body.empty()) {
            return "{\"error\":\"No body provided\"}";
        }
        
        // Validate JSON size
        if (body.size() > 100*1024) { // 100KB limit
            return "{\"error\":\"Request too large\"}";
        }
        
        QJsonParseError error;
        auto document = QJsonDocument::fromJson(QByteArray::fromStdString(body), &error);
        
        if (error.error != QJsonParseError::NoError) {
            return QString("{\"error\":\"Invalid JSON: %1\"}").arg(error.errorString()).toStdString();
        }
        
        auto requestObj = document.object();
        auto messages = requestObj["messages"].toArray();
        
        if (messages.isEmpty()) {
            return "{\"error\":\"No messages provided\"}";
        }
        
        // AI Agent session management
        QString session_id = requestObj["session_id"].toString();
        bool use_session = !session_id.isEmpty();
        GenerationParams params;
        
        // Parse generation parameters for AI Agent
        if (requestObj.contains("temperature")) {
            params.temperature = requestObj["temperature"].toDouble();
        }
        if (requestObj.contains("max_tokens")) {
            params.max_tokens = requestObj["max_tokens"].toInt();
        }
        if (requestObj.contains("top_p")) {
            params.top_p = requestObj["top_p"].toDouble();
        }
        if (requestObj.contains("stream")) {
            params.stream = requestObj["stream"].toBool();
        }
        
        // Model selection for this request
        QString requested_model = requestObj["model"].toString();
        ModelInfo* model_to_use = m_currentModel;
        
        if (!requested_model.isEmpty() && requested_model != "gpt4all") {
            for (auto& model : m_availableModels) {
                if (QString::fromStdString(model.name).contains(requested_model, Qt::CaseInsensitive)) {
                    model_to_use = &model;
                    break;
                }
            }
        }
        
        ConversationSession* session = nullptr;
        if (use_session) {
            session = &getOrCreateSession(session_id.toStdString());
            cleanupOldSessions(); // Periodic cleanup
        }
        
        // Build conversation context for AI Agent
        QString full_prompt;
        if (session && !session->system_prompt.empty()) {
            full_prompt += QString::fromStdString(session->system_prompt) + "\n\n";
        }
        
        // Add conversation history if using sessions
        if (session) {
            for (const auto& [role, content] : session->history) {
                full_prompt += QString::fromStdString(role) + ": " + QString::fromStdString(content) + "\n";
            }
        }
        
        // Get the current user message
        QString user_message;
        for (const auto &msg : messages) {
            auto msgObj = msg.toObject();
            if (msgObj["role"].toString() == "user") {
                user_message = msgObj["content"].toString();
            }
        }
        
        if (user_message.isEmpty()) {
            return "{\"error\":\"No user message found\"}";
        }
        
        // Validate prompt length
        if (user_message.length() > 10000) { // 10k char limit
            return "{\"error\":\"Prompt too long\"}";
        }
        
        full_prompt += "user: " + user_message + "\nassistant: ";
        
        qDebug() << "Processing AI Agent request:";
        qDebug() << "  Session:" << (session ? QString::fromStdString(session->session_id) : "none");
        qDebug() << "  Model:" << (model_to_use ? QString::fromStdString(model_to_use->name) : "none");
        qDebug() << "  Temperature:" << params.temperature;
        qDebug() << "  Prompt:" << user_message.left(100) << "...";
        
        // Generate AI response with enhanced context
        QString aiResponse;
        try {
            aiResponse = generateResponse(full_prompt, params, model_to_use);
        } catch (const std::exception& e) {
            return QString("{\"error\":\"Generation failed: %1\"}").arg(e.what()).toStdString();
        }
        
        // Update session history
        if (session) {
            session->history.push_back({"user", user_message.toStdString()});
            session->history.push_back({"assistant", aiResponse.toStdString()});
            
            // Limit history size to prevent memory bloat
            if (session->history.size() > 50) { // Keep last 25 exchanges
                session->history.erase(session->history.begin(), session->history.begin() + 10);
            }
        }
        
        // Format as OpenAI-compatible response with AI Agent extensions
        QJsonObject chatResponse;
        chatResponse["id"] = "chatcmpl-" + QString::number(QDateTime::currentSecsSinceEpoch());
        chatResponse["object"] = "chat.completion";
        chatResponse["created"] = QDateTime::currentSecsSinceEpoch();
        chatResponse["model"] = model_to_use ? QString::fromStdString(model_to_use->name) : "gpt4all";
        
        if (session) {
            chatResponse["session_id"] = QString::fromStdString(session->session_id);
            chatResponse["conversation_length"] = static_cast<int>(session->history.size());
        }
        
        QJsonObject choice;
        QJsonObject message;
        message["role"] = "assistant";
        message["content"] = aiResponse;
        choice["index"] = 0;
        choice["message"] = message;
        choice["finish_reason"] = "stop";
        
        QJsonArray choices;
        choices.append(choice);
        chatResponse["choices"] = choices;
        
        QJsonObject usage;
        usage["prompt_tokens"] = full_prompt.length() / 4;
        usage["completion_tokens"] = aiResponse.length() / 4;
        usage["total_tokens"] = usage["prompt_tokens"].toInt() + usage["completion_tokens"].toInt();
        chatResponse["usage"] = usage;
        
        // Add AI Agent metadata
        QJsonObject ai_agent_meta;
        ai_agent_meta["model_type"] = model_to_use ? QString::fromStdString(model_to_use->type) : "unknown";
        ai_agent_meta["temperature"] = params.temperature;
        ai_agent_meta["session_active"] = use_session;
        chatResponse["ai_agent"] = ai_agent_meta;
        
        QJsonDocument responseDoc(chatResponse);
        return responseDoc.toJson().toStdString();
    }
    
    // SIMPLIFIED REAL AI GENERATION - Direct backend approach
    QString generateResponse(const QString &prompt, const GenerationParams& params = GenerationParams(), ModelInfo* model = nullptr) {
        // Use specified model or current model  
        ModelInfo* active_model = model ? model : m_currentModel;
        
        // Validate input
        if (prompt.trimmed().isEmpty()) {
            throw std::invalid_argument("Empty prompt provided");
        }
        
        if (prompt.length() > 50000) { // 50k char hard limit
            throw std::invalid_argument("Prompt exceeds maximum length");
        }
        
        qDebug() << "🤖 AI Agent generating REAL response with:";
        qDebug() << "  Model:" << (active_model ? QString::fromStdString(active_model->name) : "default");
        qDebug() << "  Path:" << (active_model ? QString::fromStdString(active_model->path) : "none");
        qDebug() << "  Temperature:" << params.temperature;
        qDebug() << "  Max tokens:" << params.max_tokens;
        qDebug() << "  Prompt:" << prompt.left(100) << "...";
        
        QString response;
        
        if (!active_model || active_model->path.empty()) {
            response = "Error: No AI model available. Please ensure GPT4All models are in your model directory.";
        } else {
            // REAL AI INFERENCE USING LLMODEL
            response = generateRealAIResponse(prompt, active_model, params);
        }
        
        // Post-process response
        response = response.trimmed();
        
        // Validate response length
        if (response.length() > 100000) { // 100k char limit
            response = response.left(100000) + "... [truncated]";
        }
        
        // Note: Removed overly aggressive content filtering that was replacing 
        // real AI responses with hardcoded messages. Real AI models already 
        // have built-in safety measures. If additional filtering is needed,
        // it should be implemented as post-processing without replacing responses.
        
        qDebug() << "✅ AI Agent REAL response generated, length:" << response.length();
        return response;
    }
    
    QString generateRealAIResponse(const QString &prompt, ModelInfo* model, const GenerationParams& params) {
        qDebug() << "🔥 CALLING REAL GPT4ALL BACKEND INFERENCE";
        qDebug() << "   Model path:" << QString::fromStdString(model->path);
        
        const char* error = nullptr;
        QString accumulator;
        
        try {
            // Create real LLModel instance using C API with specific backend
            // Try different backends until one works
            const char* backends[] = {"cpu", "auto", nullptr};
            llmodel_model llmodel = nullptr;
            
            for (int i = 0; backends[i] != nullptr; i++) {
                error = nullptr;
                llmodel = llmodel_model_create2(model->path.c_str(), backends[i], &error);
                if (llmodel) {
                    qDebug() << "✅ LLModel created successfully with backend:" << backends[i];
                    break;
                }
                qWarning() << "❌ Failed to create model with backend" << backends[i] << ":" << (error ? error : "unknown error");
            }
            
            if (!llmodel) {
                QString errorMsg = error ? QString("Model creation error: %1").arg(error) : "Failed to create model with any backend";
                qWarning() << "❌" << errorMsg;
                return errorMsg;
            }
            
            // Load the model
            if (!llmodel_loadModel(llmodel, model->path.c_str(), 2048, -1)) {
                qWarning() << "❌ Failed to load model weights";
                llmodel_model_destroy(llmodel);
                return "Error: Failed to load model weights for inference";
            }
            
            qDebug() << "✅ Model weights loaded successfully";
            
            // Check if model is loaded
            if (!llmodel_isModelLoaded(llmodel)) {
                qWarning() << "❌ Model not properly loaded";
                llmodel_model_destroy(llmodel);
                return "Error: Model not properly loaded";
            }
            
            // Set up prompt context with real parameters
            llmodel_prompt_context ctx = {};
            ctx.n_predict = params.max_tokens;
            ctx.top_k = 40;
            ctx.top_p = params.top_p;
            ctx.min_p = 0.0f;
            ctx.temp = params.temperature;
            ctx.n_batch = 9;
            ctx.repeat_penalty = 1.10f;
            ctx.repeat_last_n = 64;
            ctx.context_erase = 0.5f;
            
            std::string prompt_str = prompt.toStdString();
            
            qDebug() << "🚀 Starting REAL AI inference...";
            
            // Set up global accumulator for callbacks
            g_ai_accumulator = &accumulator;
            
            // REAL AI INFERENCE CALL
            bool success = llmodel_prompt(llmodel, 
                                        prompt_str.c_str(),
                                        ai_prompt_callback,
                                        ai_response_callback,
                                        &ctx,
                                        &error);
            
            // Reset global accumulator
            g_ai_accumulator = nullptr;
            
            // Clean up model
            llmodel_model_destroy(llmodel);
            
            if (!success) {
                QString errorMsg = error ? QString("Inference error: %1").arg(error) : "AI inference failed";
                qWarning() << "❌" << errorMsg;
                return errorMsg;
            }
            
            qDebug() << "✅ REAL AI inference completed";
            qDebug() << "   Generated text length:" << accumulator.length();
            
            if (accumulator.isEmpty()) {
                qWarning() << "⚠️  AI model generated empty response";
                return QString("Error: AI model generated empty response for prompt: %1").arg(prompt.left(50));
            }
            
            return accumulator;
            
        } catch (const std::exception& e) {
            qWarning() << "❌ AI inference exception:" << e.what();
            return QString("Error: AI inference failed: %1").arg(e.what());
        } catch (...) {
            qWarning() << "❌ Unknown AI inference error";
            return "Error: Unknown error during AI inference";
        }
    }
    
    // REMOVED: containsHarmfulContent() function that was replacing real AI responses
    // with hardcoded messages. Real AI models have built-in safety measures.
    
private:
    struct MHD_Daemon *m_daemon;
    // AI Agent Persona Core state management
    std::vector<ModelInfo> m_availableModels;
    ModelInfo* m_currentModel = nullptr;
    std::unordered_map<std::string, ConversationSession> m_sessions;
    std::string m_defaultPersona = "You are a helpful AI assistant with access to various tools and capabilities.";
    
    // Session management for AI Agent
    std::string generateSessionId() {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return "session_" + std::to_string(ms);
    }
    
    ConversationSession& getOrCreateSession(const std::string& session_id) {
        if (m_sessions.find(session_id) == m_sessions.end()) {
            ConversationSession session;
            session.session_id = session_id;
            session.system_prompt = m_defaultPersona;
            session.last_activity = std::chrono::system_clock::now();
            m_sessions[session_id] = session;
        }
        m_sessions[session_id].last_activity = std::chrono::system_clock::now();
        return m_sessions[session_id];
    }
    
    void cleanupOldSessions() {
        auto now = std::chrono::system_clock::now();
        auto cutoff = now - std::chrono::hours(24); // 24 hour session timeout
        
        for (auto it = m_sessions.begin(); it != m_sessions.end();) {
            if (it->second.last_activity < cutoff) {
                it = m_sessions.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // AI AGENT SESSION MANAGEMENT
    std::string handleSessionManagement(const std::string &body) {
        if (body.empty()) {
            return "{\"error\":\"No body provided\"}";
        }
        
        QJsonParseError error;
        auto document = QJsonDocument::fromJson(QByteArray::fromStdString(body), &error);
        
        if (error.error != QJsonParseError::NoError) {
            return QString("{\"error\":\"Invalid JSON: %1\"}").arg(error.errorString()).toStdString();
        }
        
        auto requestObj = document.object();
        QString action = requestObj["action"].toString();
        
        if (action == "create") {
            std::string session_id = generateSessionId();
            auto& session = getOrCreateSession(session_id);
            
            // Set initial persona if provided
            if (requestObj.contains("persona")) {
                session.persona = requestObj["persona"].toString().toStdString();
                session.system_prompt = session.persona;
            }
            
            QJsonObject response;
            response["session_id"] = QString::fromStdString(session_id);
            response["created"] = QDateTime::currentSecsSinceEpoch();
            response["persona"] = QString::fromStdString(session.persona);
            
            QJsonDocument doc(response);
            return doc.toJson().toStdString();
            
        } else if (action == "list") {
            QJsonArray sessions;
            for (const auto& [id, session] : m_sessions) {
                QJsonObject sessionObj;
                sessionObj["session_id"] = QString::fromStdString(id);
                sessionObj["persona"] = QString::fromStdString(session.persona);
                sessionObj["message_count"] = static_cast<int>(session.history.size());
                sessions.append(sessionObj);
            }
            
            QJsonObject response;
            response["sessions"] = sessions;
            response["total"] = static_cast<int>(m_sessions.size());
            
            QJsonDocument doc(response);
            return doc.toJson().toStdString();
            
        } else if (action == "delete") {
            QString session_id = requestObj["session_id"].toString();
            m_sessions.erase(session_id.toStdString());
            
            return "{\"success\":true,\"message\":\"Session deleted\"}";
        }
        
        return "{\"error\":\"Invalid action. Use 'create', 'list', or 'delete'\"}";
    }
    
    // AI AGENT PERSONA MANAGEMENT
    std::string handlePersonaUpdate(const std::string &body) {
        if (body.empty()) {
            return "{\"error\":\"No body provided\"}";
        }
        
        QJsonParseError error;
        auto document = QJsonDocument::fromJson(QByteArray::fromStdString(body), &error);
        
        if (error.error != QJsonParseError::NoError) {
            return QString("{\"error\":\"Invalid JSON: %1\"}").arg(error.errorString()).toStdString();
        }
        
        auto requestObj = document.object();
        QString session_id = requestObj["session_id"].toString();
        QString persona = requestObj["persona"].toString();
        
        if (session_id.isEmpty()) {
            return "{\"error\":\"session_id required\"}";
        }
        
        auto& session = getOrCreateSession(session_id.toStdString());
        session.persona = persona.toStdString();
        session.system_prompt = persona.toStdString();
        
        QJsonObject response;
        response["success"] = true;
        response["session_id"] = session_id;
        response["persona"] = persona;
        response["message"] = "Persona updated successfully";
        
        QJsonDocument doc(response);
        return doc.toJson().toStdString();
    }
    
    // AI AGENT MODEL SELECTION
    std::string handleModelSelection(const std::string &body) {
        if (body.empty()) {
            return "{\"error\":\"No body provided\"}";
        }
        
        QJsonParseError error;
        auto document = QJsonDocument::fromJson(QByteArray::fromStdString(body), &error);
        
        if (error.error != QJsonParseError::NoError) {
            return QString("{\"error\":\"Invalid JSON: %1\"}").arg(error.errorString()).toStdString();
        }
        
        auto requestObj = document.object();
        QString model_name = requestObj["model"].toString();
        QString model_type = requestObj["type"].toString(); // "code", "chat", "reasoning"
        
        ModelInfo* selected_model = nullptr;
        
        // Find model by name or type
        if (!model_name.isEmpty()) {
            for (auto& model : m_availableModels) {
                if (QString::fromStdString(model.name).contains(model_name, Qt::CaseInsensitive)) {
                    selected_model = &model;
                    break;
                }
            }
        } else if (!model_type.isEmpty()) {
            for (auto& model : m_availableModels) {
                if (model.type == model_type.toStdString()) {
                    selected_model = &model;
                    break;
                }
            }
        }
        
        if (!selected_model) {
            return "{\"error\":\"Model not found\"}";
        }
        
        m_currentModel = selected_model;
        
        QJsonObject response;
        response["success"] = true;
        response["model"] = QString::fromStdString(selected_model->name);
        response["type"] = QString::fromStdString(selected_model->type);
        response["path"] = QString::fromStdString(selected_model->path);
        response["message"] = "Model selected successfully";
        
        QJsonDocument doc(response);
        return doc.toJson().toStdString();
    }
};

// Static member definition
std::atomic<bool> GPT4AllAPIServer::s_shutdownRequested{false};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    // Handle shutdown signals
    signal(SIGINT, [](int) { 
        qDebug() << "🛑 AI Agent Server shutting down...";
        GPT4AllAPIServer::s_shutdownRequested = true; 
        QCoreApplication::quit(); 
    });
    signal(SIGTERM, [](int) { 
        qDebug() << "🛑 AI Agent Server terminating...";
        GPT4AllAPIServer::s_shutdownRequested = true; 
        QCoreApplication::quit(); 
    });
    
    std::cout << "🚀 Starting GPT4All AI Agent Persona Core Server..." << std::endl;
    std::cout << "🔗 Features: Session Management, Model Selection, Persona Injection" << std::endl;
    std::cout << "🎯 Target: AI Agent with arsenal of scripts and bots" << std::endl;
    
    try {
        GPT4AllAPIServer server;
        
        if (!server.start(8080)) {
            std::cerr << "❌ Failed to start AI Agent server!" << std::endl;
            return 1;
        }
        
        std::cout << "✅ AI Agent Persona Core Server running on http://localhost:8080" << std::endl;
        std::cout << "📋 Available Endpoints:" << std::endl;
        std::cout << "  • POST /v1/chat/completions - Enhanced chat with sessions & personas" << std::endl;
        std::cout << "  • POST /v1/sessions - Session management (create/list/delete)" << std::endl;
        std::cout << "  • POST /v1/persona - Update persona prompts" << std::endl;
        std::cout << "  • POST /v1/models - Model selection and info" << std::endl;
        std::cout << "  • GET /health - Server health check" << std::endl;
        std::cout << "🔄 Press Ctrl+C to stop the server..." << std::endl;
        
        return app.exec();
        
    } catch (const std::exception& e) {
        std::cerr << "💥 AI Agent Server error: " << e.what() << std::endl;
        return 1;
    }
}
