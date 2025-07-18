// =============================================================================
// COMPLETE AI-ENHANCED SERVER IMPLEMENTATION
// Enhanced QTcpHttpServer with full GPT4All ChatLLM integration
// =============================================================================

#include "enhanced_ai_server.h"
#include "chatllm.h"
#include "modellist.h"
#include "database.h"
#include "chat.h"
#include "chatmodel.h"

#include <QTcpSocket>
#include <QNetworkInterface>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QJsonParseError>
#include <QCoreApplication>
#include <QUuid>

EnhancedAIServer::EnhancedAIServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_chatLLM(nullptr)
    , m_modelList(nullptr)
    , m_database(nullptr)
    , m_chat(nullptr)
{
    connect(m_server, &QTcpServer::newConnection, this, &EnhancedAIServer::onNewConnection);
    setupDefaultRoutes();
}

EnhancedAIServer::~EnhancedAIServer()
{
    close();
}

bool EnhancedAIServer::listen(const QHostAddress &address, quint16 port)
{
    if (m_server->isListening()) {
        close();
    }

    bool success = m_server->listen(address, port);
    if (success) {
        qDebug() << "EnhancedAIServer: Listening on" << address.toString() << "port" << m_server->serverPort();
        qDebug() << "🚀 REAL AI API available at: http://localhost:" << m_server->serverPort();
        qDebug() << "  Health check: http://localhost:" << m_server->serverPort() << "/";
        qDebug() << "  Models: http://localhost:" << m_server->serverPort() << "/v1/models";
        qDebug() << "  Chat completions: http://localhost:" << m_server->serverPort() << "/v1/chat/completions";
        emit requestReceived("SERVER", QString("Started on port %1").arg(m_server->serverPort()));
    } else {
        QString error = QString("Failed to start server: %1").arg(m_server->errorString());
        qWarning() << "EnhancedAIServer:" << error;
        emit errorOccurred(error);
    }
    return success;
}

void EnhancedAIServer::close()
{
    if (m_server->isListening()) {
        m_server->close();
        qDebug() << "EnhancedAIServer: Server closed";
    }
}

bool EnhancedAIServer::isListening() const
{
    return m_server->isListening();
}

quint16 EnhancedAIServer::serverPort() const
{
    return m_server->serverPort();
}

// =============================================================================
// REAL AI INTEGRATION - THE CRITICAL DIFFERENCE
// =============================================================================

void EnhancedAIServer::setChatLLM(ChatLLM *chatLLM)
{
    if (m_chatLLM) {
        // Disconnect old signals
        disconnect(m_chatLLM, nullptr, this, nullptr);
    }
    
    m_chatLLM = chatLLM;
    
    if (m_chatLLM) {
        // Connect to AI signals for real-time response handling
        connect(m_chatLLM, &ChatLLM::responseChanged, 
                this, &EnhancedAIServer::handleAIResponseChanged);
        connect(m_chatLLM, &ChatLLM::responseStopped, 
                this, &EnhancedAIServer::handleAIResponseStopped);
        connect(m_chatLLM, &ChatLLM::modelLoadingPercentageChanged,
                this, &EnhancedAIServer::handleModelLoadingPercentageChanged);
        connect(m_chatLLM, &ChatLLM::modelLoadingError,
                this, &EnhancedAIServer::handleModelLoadingError);
        
        qDebug() << "✅ EnhancedAIServer: Real ChatLLM integration connected!";
    }
}

void EnhancedAIServer::setModelList(ModelList *modelList)
{
    m_modelList = modelList;
    qDebug() << "✅ EnhancedAIServer: ModelList connected";
}

void EnhancedAIServer::setDatabase(Database *database)
{
    m_database = database;
    qDebug() << "✅ EnhancedAIServer: Database connected";
}

void EnhancedAIServer::setChat(Chat *chat)
{
    m_chat = chat;
    qDebug() << "✅ EnhancedAIServer: Chat context connected";
}

// =============================================================================
// REAL AI RESPONSE HANDLERS - THE CORE FUNCTIONALITY
// =============================================================================

void EnhancedAIServer::handleAIResponseChanged()
{
    if (!m_chatLLM || !m_chat) return;
    
    // Get the current response from ChatModel
    ChatModel *chatModel = m_chat->chatModel();
    if (!chatModel) return;
    
    // Find the current response item
    for (int i = chatModel->rowCount() - 1; i >= 0; --i) {
        auto index = chatModel->index(i);
        bool isCurrentResponse = chatModel->data(index, ChatModel::IsCurrentResponseRole).toBool();
        
        if (isCurrentResponse) {
            QString currentResponse = chatModel->data(index, ChatModel::ContentRole).toString();
            
            QMutexLocker locker(&m_sessionMutex);
            
            // Update all active streaming sessions
            for (auto &session : m_activeSessions) {
                if (session.isStreaming && session.socket && !session.isComplete) {
                    // Calculate new tokens since last update
                    QString newTokens = currentResponse.mid(session.accumulatedResponse.length());
                    session.accumulatedResponse = currentResponse;
                    
                    if (!newTokens.isEmpty()) {
                        // Send streaming chunk
                        QJsonObject chunk = createStreamingChunk(session.sessionId, newTokens, false);
                        
                        QByteArray responseData = createHttpResponse({
                            .statusCode = 200,
                            .headers = {
                                {"Content-Type", "text/plain; charset=utf-8"},
                                {"Cache-Control", "no-cache"},
                                {"Connection", "keep-alive"},
                                {"Access-Control-Allow-Origin", "*"}
                            },
                            .body = QJsonDocument(chunk).toJson(QJsonDocument::Compact) + "\n\n"
                        });
                        
                        session.socket->write(responseData);
                        session.socket->flush();
                        
                        qDebug() << "📤 Streaming chunk sent:" << newTokens.left(50);
                    }
                }
            }
            break;
        }
    }
}

void EnhancedAIServer::handleAIResponseStopped(qint64 promptResponseMs)
{
    Q_UNUSED(promptResponseMs);
    
    if (!m_chatLLM || !m_chat) return;
    
    ChatModel *chatModel = m_chat->chatModel();
    if (!chatModel) return;
    
    // Get the final response
    QString finalResponse;
    for (int i = chatModel->rowCount() - 1; i >= 0; --i) {
        auto index = chatModel->index(i);
        bool isCurrentResponse = chatModel->data(index, ChatModel::IsCurrentResponseRole).toBool();
        
        if (isCurrentResponse) {
            finalResponse = chatModel->data(index, ChatModel::ContentRole).toString();
            break;
        }
    }
    
    QMutexLocker locker(&m_sessionMutex);
    
    // Complete all active sessions
    for (auto &session : m_activeSessions) {
        if (!session.isComplete) {
            session.accumulatedResponse = finalResponse;
            session.isComplete = true;
            
            if (session.socket) {
                if (session.isStreaming) {
                    // Send final streaming chunk
                    QJsonObject finalChunk = createStreamingChunk(session.sessionId, "", true);
                    QByteArray finalData = createHttpResponse({
                        .statusCode = 200,
                        .headers = {
                            {"Content-Type", "text/plain; charset=utf-8"},
                            {"Cache-Control", "no-cache"},
                            {"Connection", "close"},
                            {"Access-Control-Allow-Origin", "*"}
                        },
                        .body = QJsonDocument(finalChunk).toJson(QJsonDocument::Compact) + "\n\n"
                    });
                    
                    session.socket->write(finalData);
                    session.socket->flush();
                    session.socket->disconnectFromHost();
                } else {
                    // Send complete response
                    QJsonObject response = createCompletionResponse(session.sessionId, finalResponse, true);
                    QByteArray responseData = createHttpResponse({
                        .statusCode = 200,
                        .headers = {{"Content-Type", "application/json"}, {"Access-Control-Allow-Origin", "*"}},
                        .body = QJsonDocument(response).toJson(QJsonDocument::Compact)
                    });
                    
                    session.socket->write(responseData);
                    session.socket->flush();
                    session.socket->disconnectFromHost();
                }
            }
            
            emit aiResponseGenerated(session.sessionId, finalResponse);
            qDebug() << "✅ AI generation completed for session:" << session.sessionId;
        }
    }
    
    // Cleanup completed sessions
    auto it = m_activeSessions.begin();
    while (it != m_activeSessions.end()) {
        if (it->isComplete) {
            it = m_activeSessions.erase(it);
        } else {
            ++it;
        }
    }
}

void EnhancedAIServer::handleModelLoadingPercentageChanged(float percentage)
{
    qDebug() << "📊 Model loading:" << (percentage * 100) << "%";
}

void EnhancedAIServer::handleModelLoadingError(const QString &error)
{
    qWarning() << "❌ Model loading error:" << error;
}

// =============================================================================
// REAL AI GENERATION - THE CORE FUNCTIONALITY
// =============================================================================

QString EnhancedAIServer::startAIGeneration(const QJsonObject &request, QTcpSocket *socket)
{
    if (!m_chatLLM || !m_chat) {
        qWarning() << "❌ No ChatLLM or Chat available for AI generation";
        return QString();
    }
    
    // Load model if needed
    QString modelName = request.value("model").toString("gpt4all-local");
    if (!loadModelIfNeeded(modelName)) {
        qWarning() << "❌ Failed to load model:" << modelName;
        return QString();
    }
    
    // Extract messages and setup chat context
    QJsonArray messages = request.value("messages").toArray();
    if (messages.isEmpty()) {
        qWarning() << "❌ No messages provided";
        return QString();
    }
    
    // Setup chat for the prompt
    setupChatForPrompt(messages);
    
    // Create generation session
    bool isStreaming = request.value("stream").toBool(false);
    QString sessionId = createGenerationSession(request, socket, isStreaming);
    
    // Start the REAL AI generation
    QStringList enabledCollections; // Empty for now, could be configured
    
    qDebug() << "🚀 Starting REAL AI generation with prompt...";
    m_chatLLM->prompt(enabledCollections);
    
    return sessionId;
}

void EnhancedAIServer::setupChatForPrompt(const QJsonArray &messages)
{
    if (!m_chat) return;
    
    ChatModel *chatModel = m_chat->chatModel();
    if (!chatModel) return;
    
    // Clear previous conversation - THIS IS SIMPLIFIED
    // In production, you'd want to manage conversation history properly
    chatModel->clear();
    
    // Add messages to chat model
    for (const auto &messageValue : messages) {
        QJsonObject message = messageValue.toObject();
        QString role = message.value("role").toString();
        QString content = message.value("content").toString();
        
        if (role == "system") {
            // Add system message (implementation depends on ChatModel structure)
            chatModel->appendPrompt(content);
        } else if (role == "user") {
            // Add user message
            chatModel->appendPrompt(content);
        } else if (role == "assistant") {
            // Add assistant message (for conversation history)
            chatModel->appendResponse(content, false);
        }
    }
    
    // Add a new response item that will be populated by AI
    chatModel->appendResponse("", true); // Empty response, marked as current
}

bool EnhancedAIServer::loadModelIfNeeded(const QString &modelName)
{
    if (!m_chatLLM || !m_modelList) {
        return false;
    }
    
    // Check if model is already loaded
    if (m_chatLLM->isModelLoaded()) {
        ModelInfo currentModel = m_chatLLM->modelInfo();
        if (currentModel.name() == modelName || currentModel.filename() == modelName) {
            return true;
        }
    }
    
    // Find and load the model
    const auto &models = m_modelList->models();
    for (const auto &model : models) {
        if (model.name() == modelName || model.filename() == modelName) {
            qDebug() << "🔄 Loading model:" << model.name();
            return m_chatLLM->loadModel(model);
        }
    }
    
    qWarning() << "❌ Model not found:" << modelName;
    return false;
}

QString EnhancedAIServer::createGenerationSession(const QJsonObject &request, QTcpSocket *socket, bool streaming)
{
    QString sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QMutexLocker locker(&m_sessionMutex);
    
    GenerationSession session;
    session.sessionId = sessionId;
    session.requestId = QString("chatcmpl-%1").arg(QDateTime::currentMSecsSinceEpoch());
    session.socket = socket;
    session.originalRequest = request;
    session.accumulatedResponse = "";
    session.isStreaming = streaming;
    session.isComplete = false;
    session.startTime = QDateTime::currentDateTime();
    
    m_activeSessions[sessionId] = session;
    
    if (socket) {
        m_socketToSession[socket] = sessionId;
    }
    
    qDebug() << "📝 Created generation session:" << sessionId << "(streaming:" << streaming << ")";
    
    return sessionId;
}

// =============================================================================
// HTTP ENDPOINTS WITH REAL AI
// =============================================================================

void EnhancedAIServer::addRoute(HttpMethod method, const QString &path, RequestHandler handler)
{
    QMutexLocker locker(&m_routesMutex);
    QString key = QString("%1:%2").arg(methodToString(method), path);
    m_routes[key] = handler;
}

void EnhancedAIServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        
        connect(client, &QTcpSocket::readyRead, this, &EnhancedAIServer::onClientDataReady);
        connect(client, &QTcpSocket::disconnected, this, &EnhancedAIServer::onClientDisconnected);
        connect(client, &QTcpSocket::disconnected, client, &QTcpSocket::deleteLater);
        
        qDebug() << "🔗 New client connected from" << client->peerAddress().toString();
    }
}

void EnhancedAIServer::onClientDataReady()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray data = client->readAll();
    qDebug() << "📨 Received" << data.size() << "bytes from" << client->peerAddress().toString();

    try {
        HttpRequest request = parseHttpRequest(data);
        
        // Route the request
        QMutexLocker locker(&m_routesMutex);
        QString key = QString("%1:%2").arg(methodToString(request.method), request.path);
        
        if (m_routes.contains(key)) {
            HttpResponse response = m_routes[key](request);
            
            if (!response.isStreaming) {
                QByteArray responseData = createHttpResponse(response);
                client->write(responseData);
                client->flush();
                client->disconnectFromHost();
            }
            // For streaming responses, the connection is kept alive
            
        } else {
            // Default 404 response
            HttpResponse response;
            response.statusCode = 404;
            response.headers["Content-Type"] = "application/json";
            response.body = R"({"error": "Not Found", "message": "The requested endpoint was not found"})";
            
            QByteArray responseData = createHttpResponse(response);
            client->write(responseData);
            client->flush();
            client->disconnectFromHost();
        }
        
        emit requestReceived(methodToString(request.method), request.path);
        
    } catch (const std::exception &e) {
        qWarning() << "❌ Error processing request:" << e.what();
        
        HttpResponse errorResponse;
        errorResponse.statusCode = 400;
        errorResponse.headers["Content-Type"] = "application/json";
        errorResponse.body = QString(R"({"error": "Bad Request", "message": "%1"})").arg(e.what()).toUtf8();
        
        QByteArray responseData = createHttpResponse(errorResponse);
        client->write(responseData);
        client->flush();
        client->disconnectFromHost();
    }
}

void EnhancedAIServer::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    
    QMutexLocker locker(&m_sessionMutex);
    
    // Clean up any sessions associated with this socket
    auto it = m_socketToSession.find(client);
    if (it != m_socketToSession.end()) {
        QString sessionId = it.value();
        m_activeSessions.remove(sessionId);
        m_socketToSession.erase(it);
        qDebug() << "🔌 Client disconnected, cleaned up session:" << sessionId;
    }
}

EnhancedAIServer::HttpResponse EnhancedAIServer::handleChatCompletions(const HttpRequest &request)
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
    
    // Validate required fields
    if (!requestData.contains("model") || !requestData.contains("messages")) {
        response.statusCode = 400;
        response.body = QJsonDocument(createErrorResponse("invalid_request", 
            "Missing required fields: model, messages")).toJson(QJsonDocument::Compact);
        return response;
    }
    
    // Check if we have real AI available
    if (!m_chatLLM) {
        response.statusCode = 503;
        response.body = QJsonDocument(createErrorResponse("service_unavailable", 
            "AI service not available")).toJson(QJsonDocument::Compact);
        return response;
    }
    
    qDebug() << "🤖 Starting REAL AI chat completion...";
    
    // Get the client socket from the current context
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    
    // Start REAL AI generation
    QString sessionId = startAIGeneration(requestData, clientSocket);
    
    if (sessionId.isEmpty()) {
        response.statusCode = 500;
        response.body = QJsonDocument(createErrorResponse("internal_error", 
            "Failed to start AI generation")).toJson(QJsonDocument::Compact);
        return response;
    }
    
    bool isStreaming = requestData.value("stream").toBool(false);
    
    if (isStreaming) {
        // For streaming, we return a response that keeps the connection alive
        response.statusCode = 200;
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        response.headers["Cache-Control"] = "no-cache";
        response.headers["Connection"] = "keep-alive";
        response.headers["Access-Control-Allow-Origin"] = "*";
        response.isStreaming = true;
        response.body = ""; // Streaming data will be sent via signals
        
        qDebug() << "📡 Started streaming AI response for session:" << sessionId;
        
    } else {
        // For non-streaming, the response will be sent when AI completes
        // This is handled in handleAIResponseStopped()
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.headers["Access-Control-Allow-Origin"] = "*";
        response.isStreaming = true; // Keep connection alive until AI completes
        response.body = "";
        
        qDebug() << "⏳ Started non-streaming AI response for session:" << sessionId;
    }
    
    return response;
}

EnhancedAIServer::HttpResponse EnhancedAIServer::handleModels(const HttpRequest &request)
{
    Q_UNUSED(request);
    
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    response.headers["Access-Control-Allow-Origin"] = "*";
    
    QJsonObject result;
    result["object"] = "list";
    
    QJsonArray models;
    
    if (m_modelList) {
        // Get real models from ModelList
        const auto &modelInfos = m_modelList->models();
        
        for (const auto &modelInfo : modelInfos) {
            if (modelInfo.installed) {
                QJsonObject model;
                model["id"] = modelInfo.name();
                model["object"] = "model";
                model["created"] = QDateTime::currentSecsSinceEpoch();
                model["owned_by"] = "gpt4all";
                model["filename"] = modelInfo.filename();
                model["description"] = modelInfo.description();
                
                models.append(model);
            }
        }
    } else {
        // Fallback model if no ModelList
        QJsonObject model;
        model["id"] = "gpt4all-local";
        model["object"] = "model";
        model["created"] = QDateTime::currentSecsSinceEpoch();
        model["owned_by"] = "gpt4all";
        
        models.append(model);
    }
    
    result["data"] = models;
    
    QJsonDocument doc(result);
    response.body = doc.toJson(QJsonDocument::Compact);
    
    return response;
}

EnhancedAIServer::HttpResponse EnhancedAIServer::handleHealth(const HttpRequest &request)
{
    Q_UNUSED(request);
    
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    response.headers["Access-Control-Allow-Origin"] = "*";
    
    QJsonObject status;
    status["status"] = "ok";
    status["server"] = "GPT4All-EnhancedAI";
    status["version"] = "1.0";
    status["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    status["ai_enabled"] = true;
    
    if (m_chatLLM) {
        status["model_loaded"] = m_chatLLM->isModelLoaded();
        if (m_chatLLM->isModelLoaded()) {
            status["current_model"] = m_chatLLM->modelInfo().name();
        }
    } else {
        status["model_loaded"] = false;
    }
    
    QJsonDocument doc(status);
    response.body = doc.toJson(QJsonDocument::Compact);
    
    return response;
}

EnhancedAIServer::HttpResponse EnhancedAIServer::handleCors(const HttpRequest &request)
{
    Q_UNUSED(request);
    
    HttpResponse response;
    response.statusCode = 200;
    response.headers["Content-Type"] = "text/plain";
    response.headers["Access-Control-Allow-Origin"] = "*";
    response.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
    response.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    response.body = "OK";
    
    return response;
}

// =============================================================================
// HELPER METHODS
// =============================================================================

QJsonObject EnhancedAIServer::createCompletionResponse(const QString &sessionId, const QString &response, bool isChat)
{
    QJsonObject result;
    
    QMutexLocker locker(&m_sessionMutex);
    auto it = m_activeSessions.find(sessionId);
    if (it == m_activeSessions.end()) {
        // Fallback response structure
        result["id"] = QString("chatcmpl-%1").arg(QDateTime::currentMSecsSinceEpoch());
        result["model"] = "gpt4all-local";
    } else {
        result["id"] = it->requestId;
        result["model"] = it->originalRequest.value("model").toString("gpt4all-local");
    }
    
    result["object"] = isChat ? "chat.completion" : "text_completion";
    result["created"] = QDateTime::currentSecsSinceEpoch();
    
    QJsonArray choices;
    QJsonObject choice;
    choice["index"] = 0;
    choice["finish_reason"] = "stop";
    
    if (isChat) {
        QJsonObject message;
        message["role"] = "assistant";
        message["content"] = response;
        choice["message"] = message;
    } else {
        choice["text"] = response;
    }
    
    choices.append(choice);
    result["choices"] = choices;
    
    QJsonObject usage;
    usage["prompt_tokens"] = response.length() / 4; // Rough estimate
    usage["completion_tokens"] = response.length() / 4;
    usage["total_tokens"] = usage["prompt_tokens"].toInt() + usage["completion_tokens"].toInt();
    result["usage"] = usage;
    
    return result;
}

QJsonObject EnhancedAIServer::createStreamingChunk(const QString &sessionId, const QString &deltaContent, bool isComplete)
{
    QJsonObject chunk;
    
    QMutexLocker locker(&m_sessionMutex);
    auto it = m_activeSessions.find(sessionId);
    if (it == m_activeSessions.end()) {
        chunk["id"] = QString("chatcmpl-%1").arg(QDateTime::currentMSecsSinceEpoch());
        chunk["model"] = "gpt4all-local";
    } else {
        chunk["id"] = it->requestId;
        chunk["model"] = it->originalRequest.value("model").toString("gpt4all-local");
    }
    
    chunk["object"] = "chat.completion.chunk";
    chunk["created"] = QDateTime::currentSecsSinceEpoch();
    
    QJsonArray choices;
    QJsonObject choice;
    choice["index"] = 0;
    
    if (isComplete) {
        choice["finish_reason"] = "stop";
        choice["delta"] = QJsonObject();
    } else {
        QJsonObject delta;
        if (!deltaContent.isEmpty()) {
            delta["content"] = deltaContent;
        }
        choice["delta"] = delta;
    }
    
    choices.append(choice);
    chunk["choices"] = choices;
    
    return chunk;
}

QJsonObject EnhancedAIServer::createErrorResponse(const QString &type, const QString &message)
{
    QJsonObject error;
    error["error"] = QJsonObject{
        {"type", type},
        {"message", message},
        {"code", "null"}
    };
    return error;
}

void EnhancedAIServer::setupDefaultRoutes()
{
    // Health check endpoint
    addRoute(GET, "/", [this](const HttpRequest &request) -> HttpResponse {
        return handleHealth(request);
    });
    
    // OpenAI-compatible models endpoint
    addRoute(GET, "/v1/models", [this](const HttpRequest &request) -> HttpResponse {
        return handleModels(request);
    });
    
    // OpenAI-compatible chat completions endpoint - THE REAL AI ENDPOINT
    addRoute(POST, "/v1/chat/completions", [this](const HttpRequest &request) -> HttpResponse {
        return handleChatCompletions(request);
    });
    
    // CORS preflight
    addRoute(OPTIONS, "/v1/models", [this](const HttpRequest &request) -> HttpResponse {
        return handleCors(request);
    });
    
    addRoute(OPTIONS, "/v1/chat/completions", [this](const HttpRequest &request) -> HttpResponse {
        return handleCors(request);
    });
}

// =============================================================================
// HTTP PROTOCOL IMPLEMENTATION
// =============================================================================

EnhancedAIServer::HttpRequest EnhancedAIServer::parseHttpRequest(const QByteArray &data)
{
    HttpRequest request;
    QString requestString = QString::fromUtf8(data);
    QStringList lines = requestString.split("\r\n");
    
    if (lines.isEmpty()) {
        return request;
    }
    
    // Parse request line (GET /path HTTP/1.1)
    QStringList requestLine = lines[0].split(" ");
    if (requestLine.size() >= 3) {
        request.method = stringToMethod(requestLine[0]);
        QString fullPath = requestLine[1];
        
        // Split path and query string
        int queryPos = fullPath.indexOf('?');
        if (queryPos != -1) {
            request.path = fullPath.left(queryPos);
            request.queryString = fullPath.mid(queryPos + 1);
        } else {
            request.path = fullPath;
        }
    }
    
    // Parse headers
    bool inHeaders = true;
    int bodyStart = -1;
    for (int i = 1; i < lines.size(); ++i) {
        if (lines[i].isEmpty()) {
            inHeaders = false;
            bodyStart = i + 1;
            break;
        }
        
        if (inHeaders) {
            int colonPos = lines[i].indexOf(':');
            if (colonPos > 0) {
                QString key = lines[i].left(colonPos).trimmed();
                QString value = lines[i].mid(colonPos + 1).trimmed();
                request.headers[key] = value;
            }
        }
    }
    
    // Parse body
    if (bodyStart >= 0 && bodyStart < lines.size()) {
        QStringList bodyLines = lines.mid(bodyStart);
        QString body = bodyLines.join("\r\n");
        request.body = body.toUtf8();
    }
    
    return request;
}

QByteArray EnhancedAIServer::createHttpResponse(const HttpResponse &response)
{
    QString statusText;
    switch (response.statusCode) {
        case 200: statusText = "OK"; break;
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 500: statusText = "Internal Server Error"; break;
        case 503: statusText = "Service Unavailable"; break;
        default: statusText = "Unknown"; break;
    }
    
    QString httpResponse = QString("HTTP/1.1 %1 %2\r\n").arg(response.statusCode).arg(statusText);
    
    // Add default headers
    httpResponse += "Server: GPT4All-EnhancedAI/1.0\r\n";
    
    // Add custom headers
    for (auto it = response.headers.begin(); it != response.headers.end(); ++it) {
        httpResponse += QString("%1: %2\r\n").arg(it.key(), it.value());
    }
    
    // Add content length if not streaming
    if (!response.isStreaming) {
        httpResponse += QString("Content-Length: %1\r\n").arg(response.body.size());
    }
    
    httpResponse += "\r\n";
    
    return httpResponse.toUtf8() + response.body;
}

QString EnhancedAIServer::methodToString(HttpMethod method) const
{
    switch (method) {
        case GET: return "GET";
        case POST: return "POST";
        case PUT: return "PUT";
        case DELETE: return "DELETE";
        case OPTIONS: return "OPTIONS";
        default: return "UNKNOWN";
    }
}

EnhancedAIServer::HttpMethod EnhancedAIServer::stringToMethod(const QString &method) const
{
    QString upper = method.toUpper();
    if (upper == "GET") return GET;
    if (upper == "POST") return POST;
    if (upper == "PUT") return PUT;
    if (upper == "DELETE") return DELETE;
    if (upper == "OPTIONS") return OPTIONS;
    return GET; // Default
}
