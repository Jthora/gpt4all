#include "minimal_simpleserver.h"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QStringList>

MinimalSimpleServer::MinimalSimpleServer()
    : m_server(new QTcpServer())
{
}

MinimalSimpleServer::~MinimalSimpleServer()
{
    stopServer();
    delete m_server;
}

bool MinimalSimpleServer::startServer(quint16 port)
{
    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        qDebug() << "Failed to start server:" << m_server->errorString();
        return false;
    }
    
    // Connect the newConnection signal using function pointers to avoid MOC
    QObject::connect(m_server, &QTcpServer::newConnection, [this]() {
        handleNewConnection();
    });
    
    qDebug() << "Server listening on port" << port;
    return true;
}

void MinimalSimpleServer::stopServer()
{
    if (m_server && m_server->isListening()) {
        m_server->close();
    }
}

bool MinimalSimpleServer::isListening() const
{
    return m_server && m_server->isListening();
}

void MinimalSimpleServer::handleNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        
        // Connect readyRead signal using function pointers
        QObject::connect(client, &QTcpSocket::readyRead, [this, client]() {
            handleClientData(client);
        });
        
        // Clean up when client disconnects
        QObject::connect(client, &QTcpSocket::disconnected, client, &QTcpSocket::deleteLater);
    }
}

void MinimalSimpleServer::handleClientData(QTcpSocket *client)
{
    QByteArray data = client->readAll();
    
    if (data.isEmpty()) {
        return;
    }
    
    HttpRequest request = parseHttpRequest(data);
    HttpResponse response = handleRequest(request);
    QByteArray responseData = createHttpResponse(response);
    
    client->write(responseData);
    client->flush();
    client->disconnectFromHost();
}

MinimalSimpleServer::HttpRequest MinimalSimpleServer::parseHttpRequest(const QByteArray &data)
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
        request.method = requestLine[0];
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
        if (lines[i].isEmpty() && inHeaders) {
            inHeaders = false;
            bodyStart = i + 1;
            break;
        }
        
        if (inHeaders && lines[i].contains(":")) {
            int colonPos = lines[i].indexOf(":");
            QString key = lines[i].left(colonPos).trimmed().toLower();
            QString value = lines[i].mid(colonPos + 1).trimmed();
            request.headers[key] = value;
        }
    }
    
    // Parse body if present
    if (bodyStart != -1 && bodyStart < lines.size()) {
        QStringList bodyLines = lines.mid(bodyStart);
        request.body = bodyLines.join("\r\n").toUtf8();
    }
    
    return request;
}

QByteArray MinimalSimpleServer::createHttpResponse(const HttpResponse &response)
{
    QString statusText;
    switch (response.statusCode) {
        case 200: statusText = "OK"; break;
        case 404: statusText = "Not Found"; break;
        case 405: statusText = "Method Not Allowed"; break;
        case 500: statusText = "Internal Server Error"; break;
        default: statusText = "Unknown"; break;
    }
    
    QString responseStr = QString("HTTP/1.1 %1 %2\r\n").arg(response.statusCode).arg(statusText);
    
    // Add default headers
    responseStr += "Content-Type: application/json\r\n";
    responseStr += "Access-Control-Allow-Origin: *\r\n";
    responseStr += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    responseStr += "Access-Control-Allow-Headers: Content-Type\r\n";
    responseStr += QString("Content-Length: %1\r\n").arg(response.body.size());
    
    // Add custom headers
    for (auto it = response.headers.begin(); it != response.headers.end(); ++it) {
        responseStr += QString("%1: %2\r\n").arg(it.key(), it.value());
    }
    
    responseStr += "\r\n";
    
    QByteArray result = responseStr.toUtf8() + response.body;
    return result;
}

MinimalSimpleServer::HttpResponse MinimalSimpleServer::handleRequest(const HttpRequest &request)
{
    qDebug() << "Request:" << request.method << request.path;
    
    // Handle CORS preflight
    if (request.method == "OPTIONS") {
        HttpResponse response;
        response.statusCode = 200;
        return response;
    }
    
    if (request.method == "GET") {
        if (request.path == "/health") {
            return handleHealthCheck();
        } else if (request.path == "/v1/models") {
            return handleModels();
        }
    } else if (request.method == "POST") {
        if (request.path == "/v1/chat/completions") {
            return handleChatCompletions(request);
        } else if (request.path == "/v1/completions") {
            return handleCompletions(request);
        }
    }
    
    // 404 for unknown endpoints
    HttpResponse response;
    response.statusCode = 404;
    QJsonObject error;
    error["error"] = "Not Found";
    error["message"] = QString("Endpoint %1 not found").arg(request.path);
    response.body = QJsonDocument(error).toJson(QJsonDocument::Compact);
    return response;
}

MinimalSimpleServer::HttpResponse MinimalSimpleServer::handleHealthCheck()
{
    HttpResponse response;
    response.statusCode = 200;
    
    QJsonObject healthData;
    healthData["status"] = "ok";
    healthData["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    healthData["service"] = "GPT4All Local API";
    
    response.body = QJsonDocument(healthData).toJson(QJsonDocument::Compact);
    return response;
}

MinimalSimpleServer::HttpResponse MinimalSimpleServer::handleModels()
{
    HttpResponse response;
    response.statusCode = 200;
    
    // Create a simple models response
    QJsonArray models;
    QJsonObject model1;
    model1["id"] = "gpt4all-test";
    model1["object"] = "model";
    model1["created"] = 1640995200;
    model1["owned_by"] = "gpt4all";
    models.append(model1);
    
    QJsonObject modelsResponse;
    modelsResponse["object"] = "list";
    modelsResponse["data"] = models;
    
    response.body = QJsonDocument(modelsResponse).toJson(QJsonDocument::Compact);
    return response;
}

MinimalSimpleServer::HttpResponse MinimalSimpleServer::handleChatCompletions(const HttpRequest &request)
{
    HttpResponse response;
    
    // Parse JSON request
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        response.statusCode = 400;
        QJsonObject error;
        error["error"] = "Bad Request";
        error["message"] = "Invalid JSON in request body";
        response.body = QJsonDocument(error).toJson(QJsonDocument::Compact);
        return response;
    }
    
    QJsonObject requestData = doc.object();
    
    // Validate required fields
    if (!validateChatRequest(requestData)) {
        response.statusCode = 400;
        QJsonObject error;
        error["error"] = "Bad Request";
        error["message"] = "Missing required fields: model, messages";
        response.body = QJsonDocument(error).toJson(QJsonDocument::Compact);
        return response;
    }
    
    // Check if streaming is requested
    bool stream = requestData.value("stream").toBool(false);
    if (stream) {
        return handleStreamingResponse(requestData, true);
    }
    
    // Generate chat response
    response.statusCode = 200;
    QJsonObject chatResponse = generateChatResponse(requestData);
    response.body = QJsonDocument(chatResponse).toJson(QJsonDocument::Compact);
    return response;
}

MinimalSimpleServer::HttpResponse MinimalSimpleServer::handleCompletions(const HttpRequest &request)
{
    HttpResponse response;
    
    // Parse JSON request
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        response.statusCode = 400;
        QJsonObject error;
        error["error"] = "Bad Request";
        error["message"] = "Invalid JSON in request body";
        response.body = QJsonDocument(error).toJson(QJsonDocument::Compact);
        return response;
    }
    
    QJsonObject requestData = doc.object();
    
    // Validate required fields
    if (!validateCompletionRequest(requestData)) {
        response.statusCode = 400;
        QJsonObject error;
        error["error"] = "Bad Request";
        error["message"] = "Missing required fields: model, prompt";
        response.body = QJsonDocument(error).toJson(QJsonDocument::Compact);
        return response;
    }
    
    // Check if streaming is requested
    bool stream = requestData.value("stream").toBool(false);
    if (stream) {
        return handleStreamingResponse(requestData, false);
    }
    
    // Generate completion response
    response.statusCode = 200;
    QJsonObject completionResponse = generateCompletionResponse(requestData);
    response.body = QJsonDocument(completionResponse).toJson(QJsonDocument::Compact);
    return response;
}

MinimalSimpleServer::HttpResponse MinimalSimpleServer::handleStreamingResponse(const QJsonObject &request, bool isChat)
{
    HttpResponse response;
    response.statusCode = 200;
    
    // Set SSE headers
    response.headers["Content-Type"] = "text/event-stream";
    response.headers["Cache-Control"] = "no-cache";
    response.headers["Connection"] = "keep-alive";
    
    // For now, return a simple streaming simulation
    // In a real implementation, this would connect to the actual model
    QString simulatedResponse = "This is a simulated streaming response for testing purposes.";
    
    QJsonObject streamChunk;
    streamChunk["id"] = "chatcmpl-" + QString::number(QDateTime::currentSecsSinceEpoch());
    streamChunk["object"] = isChat ? "chat.completion.chunk" : "text_completion";
    streamChunk["created"] = QDateTime::currentSecsSinceEpoch();
    streamChunk["model"] = request.value("model").toString("gpt4all-test");
    
    if (isChat) {
        QJsonArray choices;
        QJsonObject choice;
        choice["index"] = 0;
        QJsonObject delta;
        delta["role"] = "assistant";
        delta["content"] = simulatedResponse;
        choice["delta"] = delta;
        choice["finish_reason"] = "stop";
        choices.append(choice);
        streamChunk["choices"] = choices;
    } else {
        QJsonArray choices;
        QJsonObject choice;
        choice["text"] = simulatedResponse;
        choice["index"] = 0;
        choice["finish_reason"] = "stop";
        choices.append(choice);
        streamChunk["choices"] = choices;
    }
    
    QString sseData = "data: " + QJsonDocument(streamChunk).toJson(QJsonDocument::Compact) + "\n\n";
    sseData += "data: [DONE]\n\n";
    
    response.body = sseData.toUtf8();
    return response;
}

QJsonObject MinimalSimpleServer::generateChatResponse(const QJsonObject &request)
{
    QString model = request.value("model").toString("gpt4all-test");
    QJsonArray messages = request.value("messages").toArray();
    
    // Extract the last user message for response generation
    QString prompt = "Hello! How can I help you today?";
    for (auto it = messages.end() - 1; it >= messages.begin(); --it) {
        QJsonObject message = it->toObject();
        if (message.value("role").toString() == "user") {
            prompt = message.value("content").toString();
            break;
        }
    }
    
    // Generate response text (mock implementation)
    QString responseText = generateResponseText(prompt, model);
    
    // Build OpenAI-compatible response
    QJsonObject response;
    response["id"] = "chatcmpl-" + QString::number(QDateTime::currentSecsSinceEpoch());
    response["object"] = "chat.completion";
    response["created"] = QDateTime::currentSecsSinceEpoch();
    response["model"] = model;
    
    QJsonArray choices;
    QJsonObject choice;
    choice["index"] = 0;
    QJsonObject message;
    message["role"] = "assistant";
    message["content"] = responseText;
    choice["message"] = message;
    choice["finish_reason"] = "stop";
    choices.append(choice);
    response["choices"] = choices;
    
    QJsonObject usage;
    usage["prompt_tokens"] = prompt.length() / 4; // Rough estimate
    usage["completion_tokens"] = responseText.length() / 4;
    usage["total_tokens"] = usage["prompt_tokens"].toInt() + usage["completion_tokens"].toInt();
    response["usage"] = usage;
    
    return response;
}

QJsonObject MinimalSimpleServer::generateCompletionResponse(const QJsonObject &request)
{
    QString model = request.value("model").toString("gpt4all-test");
    QString prompt = request.value("prompt").toString();
    
    // Generate response text (mock implementation)
    QString responseText = generateResponseText(prompt, model);
    
    // Build OpenAI-compatible response
    QJsonObject response;
    response["id"] = "cmpl-" + QString::number(QDateTime::currentSecsSinceEpoch());
    response["object"] = "text_completion";
    response["created"] = QDateTime::currentSecsSinceEpoch();
    response["model"] = model;
    
    QJsonArray choices;
    QJsonObject choice;
    choice["text"] = responseText;
    choice["index"] = 0;
    choice["finish_reason"] = "stop";
    choices.append(choice);
    response["choices"] = choices;
    
    QJsonObject usage;
    usage["prompt_tokens"] = prompt.length() / 4; // Rough estimate
    usage["completion_tokens"] = responseText.length() / 4;
    usage["total_tokens"] = usage["prompt_tokens"].toInt() + usage["completion_tokens"].toInt();
    response["usage"] = usage;
    
    return response;
}

QString MinimalSimpleServer::generateResponseText(const QString &prompt, const QString &model)
{
    // Mock AI response generation
    // In a real implementation, this would connect to GPT4All's ChatLLM
    QStringList responses = {
        "I'm a test response from the GPT4All MinimalSimpleServer. Your prompt was: \"" + prompt + "\"",
        "This is a simulated AI response for testing purposes. How can I help you further?",
        "Thank you for testing the GPT4All local API server! This is a mock response.",
        "I understand you said: \"" + prompt + "\". I'm currently running in test mode."
    };
    
    // Simple deterministic selection based on prompt length
    int index = prompt.length() % responses.length();
    return responses[index];
}

bool MinimalSimpleServer::validateChatRequest(const QJsonObject &request)
{
    // Check required fields for chat completions
    if (!request.contains("model") || !request.contains("messages")) {
        return false;
    }
    
    QJsonValue messages = request.value("messages");
    if (!messages.isArray() || messages.toArray().isEmpty()) {
        return false;
    }
    
    // Validate message structure
    for (const QJsonValue &messageVal : messages.toArray()) {
        QJsonObject message = messageVal.toObject();
        if (!message.contains("role") || !message.contains("content")) {
            return false;
        }
    }
    
    return true;
}

bool MinimalSimpleServer::validateCompletionRequest(const QJsonObject &request)
{
    // Check required fields for completions
    return request.contains("model") && request.contains("prompt");
}
