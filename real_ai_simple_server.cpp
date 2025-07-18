#include "real_ai_simple_server.h"
#include "chat.h"

RealAISimpleServer::RealAISimpleServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_chatLLM(nullptr)
    , m_modelList(nullptr)
    , m_database(nullptr)
{
}

void RealAISimpleServer::setChatLLM(ChatLLM *chatLLM)
{
    if (m_chatLLM) {
        // Disconnect old signals
        disconnect(m_chatLLM, nullptr, this, nullptr);
    }
    
    m_chatLLM = chatLLM;
    
    if (m_chatLLM) {
        // Connect to AI signals for real-time response handling
        connect(m_chatLLM, &ChatLLM::responseChanged, 
                this, &RealAISimpleServer::handleResponseChanged);
        connect(m_chatLLM, &ChatLLM::responseStopped, 
                this, &RealAISimpleServer::handleResponseStopped);
        connect(m_chatLLM, &ChatLLM::modelLoadingPercentageChanged,
                this, &RealAISimpleServer::handleModelLoadingPercentageChanged);
        connect(m_chatLLM, &ChatLLM::modelLoadingError,
                this, &RealAISimpleServer::handleModelLoadingError);
    }
}

HttpResponse RealAISimpleServer::handleChatCompletions(const HttpRequest &request, QTcpSocket *socket)
{
    HttpResponse response;
    
    // Parse JSON request
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        response.statusCode = 400;
        response.body = QJsonDocument(createErrorResponse("parse_error", 
            "Invalid JSON in request body")).toJson(QJsonDocument::Compact);
        return response;
    }
    
    QJsonObject requestData = doc.object();
    
    // Validate request
    if (!validateChatRequest(requestData)) {
        response.statusCode = 400;
        response.body = QJsonDocument(createErrorResponse("invalid_request", 
            "Missing required fields: model, messages")).toJson(QJsonDocument::Compact);
        return response;
    }
    
    // Check if we have a real ChatLLM instance
    if (!m_chatLLM) {
        response.statusCode = 503;
        response.body = QJsonDocument(createErrorResponse("service_unavailable", 
            "AI model not available")).toJson(QJsonDocument::Compact);
        return response;
    }
    
    QString modelName = requestData.value("model").toString();
    
    // Load model if needed
    if (!loadModelIfNeeded(modelName)) {
        response.statusCode = 400;
        response.body = QJsonDocument(createErrorResponse("model_not_found", 
            QString("Model '%1' not available").arg(modelName))).toJson(QJsonDocument::Compact);
        return response;
    }
    
    // Check if streaming is requested
    bool stream = requestData.value("stream").toBool(false);
    
    if (stream) {
        // Start streaming response
        startStreamingGeneration(requestData, socket);
        
        // Return special response indicating streaming started
        response.statusCode = 200;
        response.headers["Content-Type"] = "text/event-stream";
        response.headers["Cache-Control"] = "no-cache";
        response.headers["Connection"] = "keep-alive";
        response.body = QByteArray(); // Empty - streaming will handle content
        return response;
    } else {
        // Non-streaming: generate complete response
        QString aiResponse = generateAIResponse(requestData);
        
        // Build OpenAI-compatible response
        QJsonObject chatResponse;
        chatResponse["id"] = "chatcmpl-" + generateRequestId();
        chatResponse["object"] = "chat.completion";
        chatResponse["created"] = QDateTime::currentSecsSinceEpoch();
        chatResponse["model"] = modelName;
        
        QJsonArray choices;
        QJsonObject choice;
        choice["index"] = 0;
        QJsonObject message;
        message["role"] = "assistant";
        message["content"] = aiResponse;
        choice["message"] = message;
        choice["finish_reason"] = "stop";
        choices.append(choice);
        chatResponse["choices"] = choices;
        
        // Add usage statistics
        QJsonObject usage;
        QJsonArray messages = requestData.value("messages").toArray();
        int promptTokens = 0;
        for (const auto &msg : messages) {
            promptTokens += msg.toObject().value("content").toString().length() / 4;
        }
        usage["prompt_tokens"] = promptTokens;
        usage["completion_tokens"] = aiResponse.length() / 4;
        usage["total_tokens"] = usage["prompt_tokens"].toInt() + usage["completion_tokens"].toInt();
        chatResponse["usage"] = usage;
        
        response.statusCode = 200;
        response.body = QJsonDocument(chatResponse).toJson(QJsonDocument::Compact);
        return response;
    }
}

QString RealAISimpleServer::generateAIResponse(const QJsonObject &request)
{
    if (!m_chatLLM) {
        return "Error: No AI model available";
    }
    
    // Extract messages and build prompt
    QJsonArray messages = request.value("messages").toArray();
    QString prompt;
    
    for (const auto &messageVal : messages) {
        QJsonObject message = messageVal.toObject();
        QString role = message.value("role").toString();
        QString content = message.value("content").toString();
        
        if (role == "system") {
            prompt += "System: " + content + "\n";
        } else if (role == "user") {
            prompt += "User: " + content + "\n";
        } else if (role == "assistant") {
            prompt += "Assistant: " + content + "\n";
        }
    }
    
    prompt += "Assistant: "; // Prompt for response
    
    // **CRITICAL**: This is where real AI integration happens
    // We need to call the actual ChatLLM inference engine
    
    // Set the prompt in ChatLLM
    // Note: This is a simplified version - real implementation needs proper chat context
    
    // For now, trigger the AI generation (this is the key integration point)
    // In the real implementation, you would:
    // 1. Set up the prompt in the chat context
    // 2. Call m_chatLLM->prompt() to start generation
    // 3. Wait for the response through signals
    // 4. Return the generated text
    
    // PLACEHOLDER: Real implementation would connect to ChatLLM's response system
    return "This would be the real AI response from GPT4All models";
}

bool RealAISimpleServer::loadModelIfNeeded(const QString &modelName)
{
    if (!m_chatLLM || !m_modelList) {
        return false;
    }
    
    // Check if model is already loaded
    if (m_chatLLM->modelInfo().filename() == modelName) {
        return true;
    }
    
    // Find model in available models
    ModelInfo modelInfo;
    const auto &models = m_modelList->models();
    
    for (const auto &model : models) {
        if (model.filename() == modelName || model.name() == modelName) {
            modelInfo = model;
            break;
        }
    }
    
    if (modelInfo.filename().isEmpty()) {
        return false;
    }
    
    // Load the model
    return m_chatLLM->loadModel(modelInfo);
}

void RealAISimpleServer::handleResponseChanged()
{
    // This is called when ChatLLM generates new tokens
    if (!m_chatLLM) return;
    
    QString currentResponse = m_chatLLM->response();
    
    // Handle streaming responses to connected clients
    for (auto &generation : m_activeGenerations) {
        if (generation.isStreaming && generation.socket) {
            // Calculate new tokens since last update
            QString newTokens = currentResponse.mid(generation.accumulatedResponse.length());
            generation.accumulatedResponse = currentResponse;
            
            if (!newTokens.isEmpty()) {
                // Send streaming chunk
                QJsonObject chunk;
                chunk["id"] = generation.requestId;
                chunk["object"] = "chat.completion.chunk";
                chunk["created"] = QDateTime::currentSecsSinceEpoch();
                chunk["model"] = generation.originalRequest.value("model").toString();
                
                QJsonArray choices;
                QJsonObject choice;
                choice["index"] = 0;
                QJsonObject delta;
                delta["content"] = newTokens;
                choice["delta"] = delta;
                choices.append(choice);
                chunk["choices"] = choices;
                
                sendStreamingChunk(generation.socket, chunk);
            }
        }
    }
}

void RealAISimpleServer::startStreamingGeneration(const QJsonObject &request, QTcpSocket *socket)
{
    QString requestId = generateRequestId();
    
    GenerationState state;
    state.socket = socket;
    state.requestId = requestId;
    state.originalRequest = request;
    state.accumulatedResponse = "";
    state.isStreaming = true;
    state.startTime = QDateTime::currentDateTime();
    
    m_activeGenerations[requestId] = state;
    
    // Start AI generation (this would trigger the real AI)
    // generateAIResponse(request); // This would start async generation
    
    // Send initial streaming headers
    QString headers = "HTTP/1.1 200 OK\r\n";
    headers += "Content-Type: text/event-stream\r\n";
    headers += "Cache-Control: no-cache\r\n";
    headers += "Connection: keep-alive\r\n";
    headers += "Access-Control-Allow-Origin: *\r\n\r\n";
    
    socket->write(headers.toUtf8());
    socket->flush();
}

// Additional critical methods would include:
// - Real model management
// - Proper streaming implementation  
// - Error handling
// - Context management
// - Token counting
// - Rate limiting
// etc.
