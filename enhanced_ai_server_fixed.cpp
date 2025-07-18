#include "enhanced_ai_server_fixed.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QUuid>
#include <QDebug>
#include <QRegularExpression>

EnhancedAIServerFixed::EnhancedAIServerFixed(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_chat(nullptr)
    , m_chatLLM(nullptr)
    , m_chatModel(nullptr)
    , m_modelList(nullptr)
    , m_database(nullptr)
    , m_generationInProgress(false)
{
    // CORRECTED: Proper GPT4All component initialization
    qDebug() << "Initializing Enhanced AI Server with real GPT4All integration...";
    
    m_database = new Database(this);
    m_modelList = ModelList::globalInstance();
    
    // Create Chat - this will create ChatLLM internally
    m_chat = new Chat(this);
    m_chatLLM = m_chat->chatLLM();
    m_chatModel = m_chat->chatModel();
    
    // Verify components were created
    if (!m_chatLLM) {
        qCritical() << "Failed to create ChatLLM!";
        return;
    }
    if (!m_chatModel) {
        qCritical() << "Failed to create ChatModel!";
        return;
    }
    
    // CORRECTED: Connect to real ChatLLM signals for streaming
    connect(m_chatLLM, &ChatLLM::responseChanged, 
            this, &EnhancedAIServerFixed::handleResponseChanged, Qt::QueuedConnection);
    connect(m_chatLLM, &ChatLLM::responseStopped, 
            this, &EnhancedAIServerFixed::handleResponseFinished, Qt::QueuedConnection);
    connect(m_chatLLM, &ChatLLM::responseFailed, 
            this, &EnhancedAIServerFixed::handleResponseFailed, Qt::QueuedConnection);
            
    connect(m_server, &QTcpServer::newConnection, 
            this, &EnhancedAIServerFixed::handleNewConnection);
            
    qDebug() << "Enhanced AI Server initialized successfully";
}

EnhancedAIServerFixed::~EnhancedAIServerFixed()
{
    stopServer();
    
    // Clean up sessions
    QMutexLocker locker(&m_sessionsMutex);
    for (auto* session : m_sessions) {
        if (session->timeoutTimer) {
            session->timeoutTimer->stop();
            delete session->timeoutTimer;
        }
        delete session;
    }
    m_sessions.clear();
}

bool EnhancedAIServerFixed::startServer(quint16 port)
{
    if (m_server->isListening()) {
        qWarning() << "Server is already running";
        return true;
    }
    
    if (!m_server->listen(QHostAddress::Any, port)) {
        qCritical() << "Failed to start server:" << m_server->errorString();
        return false;
    }
    
    qDebug() << "Enhanced AI Server started on port:" << port;
    qDebug() << "Available endpoints:";
    qDebug() << "  POST /v1/chat/completions - OpenAI-compatible chat completions";
    qDebug() << "  GET  /v1/models - List available models";
    qDebug() << "  Real GPT4All AI integration enabled!";
    
    return true;
}

void EnhancedAIServerFixed::stopServer()
{
    if (m_server->isListening()) {
        m_server->close();
        qDebug() << "Server stopped";
    }
}

bool EnhancedAIServerFixed::isRunning() const
{
    return m_server->isListening();
}

void EnhancedAIServerFixed::handleNewConnection()
{
    QTcpSocket* socket = m_server->nextPendingConnection();
    if (!socket) return;
    
    connect(socket, &QTcpSocket::readyRead, 
            this, &EnhancedAIServerFixed::handleReadyRead);
    connect(socket, &QTcpSocket::disconnected, 
            this, &EnhancedAIServerFixed::handleClientDisconnected);
    
    qDebug() << "New client connected from:" << socket->peerAddress().toString();
}

void EnhancedAIServerFixed::handleClientDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;
    
    // Find and clean up associated session
    QMutexLocker locker(&m_sessionsMutex);
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it.value()->socket == socket) {
            cleanupSession(it.key());
            break;
        }
    }
    
    socket->deleteLater();
    qDebug() << "Client disconnected";
}

void EnhancedAIServerFixed::handleReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;
    
    QByteArray data = socket->readAll();
    processHttpRequest(socket, data);
}

void EnhancedAIServerFixed::processHttpRequest(QTcpSocket* socket, const QByteArray& requestData)
{
    QString request = QString::fromUtf8(requestData);
    QStringList lines = request.split("\r\n");
    
    if (lines.isEmpty()) {
        socket->write(createHttpResponse(createErrorResponse("Invalid request"), "application/json", 400));
        socket->flush();
        return;
    }
    
    QString requestLine = lines[0];
    QStringList parts = requestLine.split(" ");
    
    if (parts.size() < 3) {
        socket->write(createHttpResponse(createErrorResponse("Invalid request line"), "application/json", 400));
        socket->flush();
        return;
    }
    
    QString method = parts[0];
    QString path = parts[1];
    
    // Add CORS headers
    QByteArray corsHeaders = 
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    
    // Handle OPTIONS preflight
    if (method == "OPTIONS") {
        QByteArray response = "HTTP/1.1 204 No Content\r\n" + corsHeaders + "\r\n";
        socket->write(response);
        socket->flush();
        return;
    }
    
    // Extract JSON body if present
    QJsonObject jsonRequest;
    if (method == "POST") {
        int headerEnd = request.indexOf("\r\n\r\n");
        if (headerEnd != -1) {
            QString body = request.mid(headerEnd + 4);
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(body.toUtf8(), &error);
            
            if (error.error != QJsonParseError::NoError) {
                socket->write(createHttpResponse(createErrorResponse("Invalid JSON"), "application/json", 400));
                socket->flush();
                return;
            }
            
            jsonRequest = doc.object();
        }
    }
    
    // Route the request
    if (path == "/v1/chat/completions" && method == "POST") {
        handleChatCompletions(socket, jsonRequest);
    } else if (path == "/v1/models" && method == "GET") {
        handleModelsRequest(socket);
    } else {
        socket->write(createHttpResponse(createErrorResponse("Not found"), "application/json", 404));
        socket->flush();
    }
}

void EnhancedAIServerFixed::handleChatCompletions(QTcpSocket* socket, const QJsonObject& request)
{
    // Validate request
    if (!request.contains("messages") || !request["messages"].isArray()) {
        socket->write(createHttpResponse(createErrorResponse("Missing or invalid 'messages' field"), "application/json", 400));
        socket->flush();
        return;
    }
    
    QJsonArray messages = request["messages"].toArray();
    if (messages.isEmpty()) {
        socket->write(createHttpResponse(createErrorResponse("Messages array cannot be empty"), "application/json", 400));
        socket->flush();
        return;
    }
    
    // Generate session ID
    QString sessionId = generateSessionId();
    
    // Create session
    AISession* session = new AISession();
    session->sessionId = sessionId;
    session->socket = socket;
    session->originalRequest = request;
    session->isStreaming = request["stream"].toBool(false);
    session->currentModel = request["model"].toString("gpt-3.5-turbo");
    
    // Setup timeout
    session->timeoutTimer = new QTimer();
    session->timeoutTimer->setSingleShot(true);
    session->timeoutTimer->setInterval(SESSION_TIMEOUT_MS);
    connect(session->timeoutTimer, &QTimer::timeout, this, &EnhancedAIServerFixed::handleSessionTimeout);
    session->timeoutTimer->start();
    
    {
        QMutexLocker locker(&m_sessionsMutex);
        m_sessions[sessionId] = session;
    }
    
    qDebug() << "Starting AI generation for session:" << sessionId 
             << "streaming:" << session->isStreaming
             << "model:" << session->currentModel;
    
    // Start AI generation - CORRECTED
    startAIGeneration(sessionId, request);
}

void EnhancedAIServerFixed::handleModelsRequest(QTcpSocket* socket)
{
    QJsonArray models;
    
    // Get available models from ModelList
    QStringList availableModels = getAvailableModels();
    
    for (const QString& modelName : availableModels) {
        QJsonObject model;
        model["id"] = modelName;
        model["object"] = "model";
        model["created"] = QDateTime::currentSecsSinceEpoch();
        model["owned_by"] = "gpt4all";
        models.append(model);
    }
    
    // Add default models if none found
    if (models.isEmpty()) {
        QStringList defaultModels = {"gpt-3.5-turbo", "gpt-4", "claude-3-sonnet"};
        for (const QString& modelName : defaultModels) {
            QJsonObject model;
            model["id"] = modelName;
            model["object"] = "model";
            model["created"] = QDateTime::currentSecsSinceEpoch();
            model["owned_by"] = "gpt4all";
            models.append(model);
        }
    }
    
    QJsonObject response;
    response["object"] = "list";
    response["data"] = models;
    
    socket->write(createHttpResponse(response));
    socket->flush();
}

// CORRECTED: AI Integration with proper ChatModel usage
bool EnhancedAIServerFixed::setupAIChat(const QJsonArray& messages, const QString& model)
{
    if (!m_chatModel || !m_chatLLM) {
        qWarning() << "ChatModel or ChatLLM not initialized";
        return false;
    }
    
    try {
        qDebug() << "Setting up AI chat with" << messages.size() << "messages";
        
        // Clear previous conversation
        m_chatModel->clear();
        
        // Process messages and append to chat
        for (const auto& messageValue : messages) {
            QJsonObject message = messageValue.toObject();
            QString role = message["role"].toString();
            QString content = message["content"].toString();
            
            qDebug() << "Adding message - Role:" << role << "Content length:" << content.length();
            
            if (role == "user" || role == "system") {
                // CORRECTED: Proper prompt appending
                m_chatModel->appendPrompt(content);
            }
        }
        
        // CORRECTED: Create response item properly (no parameters)
        m_chatModel->appendResponse();
        
        qDebug() << "AI chat setup completed, chat model count:" << m_chatModel->count();
        return true;
        
    } catch (const std::exception& e) {
        qWarning() << "Error setting up AI chat:" << e.what();
        return false;
    }
}

void EnhancedAIServerFixed::startAIGeneration(const QString& sessionId, const QJsonObject& request)
{
    if (m_generationInProgress) {
        qWarning() << "Generation already in progress, rejecting session:" << sessionId;
        
        QMutexLocker locker(&m_sessionsMutex);
        AISession* session = m_sessions.value(sessionId);
        if (session && session->socket) {
            session->socket->write(createHttpResponse(createErrorResponse("Server busy"), "application/json", 503));
            session->socket->flush();
        }
        cleanupSession(sessionId);
        return;
    }
    
    m_currentSessionId = sessionId;
    m_generationInProgress = true;
    
    qDebug() << "Starting AI generation for session:" << sessionId;
    
    // Load model if needed
    QString model = request["model"].toString("gpt-3.5-turbo");
    if (!loadModelIfNeeded(model)) {
        qWarning() << "Failed to load model:" << model;
        handleResponseFailed();
        return;
    }
    
    // Setup chat context
    QJsonArray messages = request["messages"].toArray();
    if (!setupAIChat(messages, model)) {
        qWarning() << "Failed to setup AI chat";
        handleResponseFailed();
        return;
    }
    
    // CORRECTED: Start real AI generation
    QStringList enabledCollections; // Empty for now, could add RAG later
    
    qDebug() << "Calling ChatLLM::prompt() for real AI generation...";
    m_chatLLM->prompt(enabledCollections);
}

// CORRECTED: Handle real AI responses
void EnhancedAIServerFixed::handleResponseChanged()
{
    if (!m_generationInProgress || m_currentSessionId.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&m_sessionsMutex);
    AISession* session = m_sessions.value(m_currentSessionId);
    if (!session || !session->socket) {
        qWarning() << "Session not found or socket invalid during response change";
        return;
    }
    
    // CORRECTED: Get response from ChatModel properly
    // The response is accumulated in the last ChatItem
    if (m_chatModel->count() > 0) {
        auto lastItem = m_chatModel->get(m_chatModel->count() - 1);
        if (lastItem && lastItem->type() == ChatItem::Type::Response) {
            QString currentResponse = lastItem->value();
            
            // Check for new content
            if (currentResponse.length() > session->accumulatedResponse.length()) {
                QString newTokens = currentResponse.mid(session->accumulatedResponse.length());
                session->accumulatedResponse = currentResponse;
                
                qDebug() << "New AI tokens received, length:" << newTokens.length() 
                         << "total:" << session->accumulatedResponse.length();
                
                // Send streaming chunk
                if (session->isStreaming) {
                    QJsonObject chunk = createStreamingChunk(m_currentSessionId, newTokens, false);
                    session->socket->write(createHttpResponse(chunk));
                    session->socket->flush();
                }
            }
        }
    }
}

void EnhancedAIServerFixed::handleResponseFinished()
{
    qDebug() << "AI response finished for session:" << m_currentSessionId;
    
    if (!m_generationInProgress || m_currentSessionId.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&m_sessionsMutex);
    AISession* session = m_sessions.value(m_currentSessionId);
    if (!session || !session->socket) {
        cleanupSession(m_currentSessionId);
        return;
    }
    
    qDebug() << "Final response length:" << session->accumulatedResponse.length();
    
    // Send final response
    if (session->isStreaming) {
        // Send end chunk
        QJsonObject endChunk = createStreamingChunk(m_currentSessionId, "", true);
        session->socket->write(createHttpResponse(endChunk));
    } else {
        // Send complete response
        QJsonObject response;
        response["id"] = m_currentSessionId;
        response["object"] = "chat.completion";
        response["created"] = QDateTime::currentSecsSinceEpoch();
        response["model"] = session->currentModel;
        
        QJsonObject message;
        message["role"] = "assistant";
        message["content"] = session->accumulatedResponse;
        
        QJsonArray choices;
        QJsonObject choice;
        choice["index"] = 0;
        choice["message"] = message;
        choice["finish_reason"] = "stop";
        choices.append(choice);
        
        response["choices"] = choices;
        
        session->socket->write(createHttpResponse(response));
    }
    
    session->socket->flush();
    cleanupSession(m_currentSessionId);
    
    m_generationInProgress = false;
    m_currentSessionId.clear();
}

void EnhancedAIServerFixed::handleResponseFailed()
{
    qWarning() << "AI response failed for session:" << m_currentSessionId;
    
    if (!m_generationInProgress || m_currentSessionId.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&m_sessionsMutex);
    AISession* session = m_sessions.value(m_currentSessionId);
    if (session && session->socket) {
        QJsonObject errorResponse = createErrorResponse("AI generation failed", "internal_error");
        session->socket->write(createHttpResponse(errorResponse, "application/json", 500));
        session->socket->flush();
    }
    
    cleanupSession(m_currentSessionId);
    m_generationInProgress = false;
    m_currentSessionId.clear();
}

void EnhancedAIServerFixed::handleSessionTimeout()
{
    QTimer* timer = qobject_cast<QTimer*>(sender());
    if (!timer) return;
    
    // Find session by timer
    QMutexLocker locker(&m_sessionsMutex);
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it.value()->timeoutTimer == timer) {
            qDebug() << "Session timeout:" << it.key();
            
            if (it.value()->socket) {
                QJsonObject errorResponse = createErrorResponse("Request timeout", "timeout");
                it.value()->socket->write(createHttpResponse(errorResponse, "application/json", 408));
                it.value()->socket->flush();
            }
            
            cleanupSession(it.key());
            break;
        }
    }
}

bool EnhancedAIServerFixed::loadModelIfNeeded(const QString& modelName)
{
    if (!m_chatLLM) {
        qWarning() << "ChatLLM not available";
        return false;
    }
    
    // Check if model is already loaded
    if (m_chatLLM->isModelLoaded()) {
        qDebug() << "Model already loaded";
        return true;
    }
    
    // Try to load from ModelList
    if (m_modelList) {
        for (int i = 0; i < m_modelList->count(); ++i) {
            const ModelInfo* modelInfo = m_modelList->get(i);
            if (modelInfo && (modelInfo->name() == modelName || 
                             modelInfo->id() == modelName ||
                             modelInfo->filename() == modelName)) {
                
                qDebug() << "Loading model:" << modelInfo->name();
                return m_chatLLM->loadModel(*modelInfo);
            }
        }
    }
    
    qDebug() << "Model not found in ModelList, proceeding with any available model";
    return true; // Allow to proceed, ChatLLM might have a default model
}

QStringList EnhancedAIServerFixed::getAvailableModels() const
{
    QStringList models;
    
    if (m_modelList) {
        for (int i = 0; i < m_modelList->count(); ++i) {
            const ModelInfo* modelInfo = m_modelList->get(i);
            if (modelInfo) {
                models << modelInfo->name();
            }
        }
    }
    
    return models;
}

void EnhancedAIServerFixed::cleanupSession(const QString& sessionId)
{
    QMutexLocker locker(&m_sessionsMutex);
    
    AISession* session = m_sessions.take(sessionId);
    if (session) {
        if (session->timeoutTimer) {
            session->timeoutTimer->stop();
            delete session->timeoutTimer;
        }
        delete session;
        qDebug() << "Session cleaned up:" << sessionId;
    }
}

QString EnhancedAIServerFixed::generateSessionId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

AISession* EnhancedAIServerFixed::findSessionBySocket(QTcpSocket* socket)
{
    QMutexLocker locker(&m_sessionsMutex);
    
    for (auto* session : m_sessions) {
        if (session->socket == socket) {
            return session;
        }
    }
    
    return nullptr;
}

QByteArray EnhancedAIServerFixed::createHttpResponse(const QJsonObject& jsonResponse, 
                                                    const QString& contentType,
                                                    int statusCode) const
{
    QJsonDocument doc(jsonResponse);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    QString statusText = (statusCode == 200) ? "OK" : 
                        (statusCode == 400) ? "Bad Request" :
                        (statusCode == 404) ? "Not Found" :
                        (statusCode == 500) ? "Internal Server Error" :
                        (statusCode == 503) ? "Service Unavailable" :
                        (statusCode == 408) ? "Request Timeout" : "Unknown";
    
    QByteArray response;
    response += QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(statusText).toUtf8();
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response += "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    response += QString("Content-Type: %1\r\n").arg(contentType).toUtf8();
    response += QString("Content-Length: %1\r\n").arg(jsonData.size()).toUtf8();
    response += "Connection: close\r\n";
    response += "\r\n";
    response += jsonData;
    
    return response;
}

QJsonObject EnhancedAIServerFixed::createErrorResponse(const QString& message, const QString& type) const
{
    QJsonObject error;
    error["message"] = message;
    error["type"] = type;
    error["code"] = "api_error";
    
    QJsonObject response;
    response["error"] = error;
    
    return response;
}

QJsonObject EnhancedAIServerFixed::createStreamingChunk(const QString& sessionId, const QString& content, bool isEnd) const
{
    QJsonObject chunk;
    chunk["id"] = sessionId;
    chunk["object"] = "chat.completion.chunk";
    chunk["created"] = QDateTime::currentSecsSinceEpoch();
    chunk["model"] = "gpt-3.5-turbo";
    
    QJsonArray choices;
    QJsonObject choice;
    choice["index"] = 0;
    
    if (isEnd) {
        choice["finish_reason"] = "stop";
        choice["delta"] = QJsonObject();
    } else {
        choice["finish_reason"] = QJsonValue::Null;
        
        QJsonObject delta;
        if (!content.isEmpty()) {
            delta["content"] = content;
        }
        choice["delta"] = delta;
    }
    
    choices.append(choice);
    chunk["choices"] = choices;
    
    return chunk;
}
