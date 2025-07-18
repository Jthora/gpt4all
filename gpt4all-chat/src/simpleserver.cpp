#include "simpleserver.h"
#include "chatllm.h"

#include <QTcpSocket>
#include <QUrl>
#include <QJsonParseError>
#include <QTimer>
#include <QThread>
#include <QCoreApplication>
#include <QMetaObject>

SimpleServer::SimpleServer()
    : m_server(new QTcpServer())
    , m_chatLLM(nullptr)
{
    // Ensure QTcpServer is in the main thread
    m_server->moveToThread(QCoreApplication::instance()->thread());
}

SimpleServer::~SimpleServer()
{
    stopServer();
    delete m_server;
}

bool SimpleServer::startServer(quint16 port)
{
    if (m_server->isListening()) {
        stopServer();
    }

    // Ensure the server operations happen in the main thread
    m_server->moveToThread(QCoreApplication::instance()->thread());

    bool success = m_server->listen(QHostAddress::LocalHost, port);
    if (success) {
        qDebug() << "SimpleServer: HTTP server started on port" << m_server->serverPort();
        qDebug() << "OpenAI-compatible API available at: http://localhost:" << m_server->serverPort();
        qDebug() << "  Health check: http://localhost:" << m_server->serverPort() << "/";
        qDebug() << "  Models: http://localhost:" << m_server->serverPort() << "/v1/models";
        qDebug() << "  Chat completions: http://localhost:" << m_server->serverPort() << "/v1/chat/completions";
        
        // Start a simple polling loop to handle connections in main thread
        QTimer *timer = new QTimer();
        timer->moveToThread(QCoreApplication::instance()->thread());
        timer->connect(timer, &QTimer::timeout, [this]() {
            if (m_server->hasPendingConnections()) {
                handleNewConnection();
            }
        });
        timer->start(100); // Check every 100ms
        
    } else {
        qWarning() << "SimpleServer: Failed to start server on port" << port << "-" << m_server->errorString();
    }
    return success;
}

void SimpleServer::stopServer()
{
    if (m_server->isListening()) {
        m_server->close();
        qDebug() << "SimpleServer: Server stopped";
    }
}

bool SimpleServer::isListening() const
{
    return m_server->isListening();
}

void SimpleServer::handleNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        
        if (client) {
            qDebug() << "SimpleServer: New client connected from" << client->peerAddress().toString();
            
            // Wait for data with timeout
            if (client->waitForReadyRead(5000)) {
                handleClientData(client);
            }
            
            client->disconnectFromHost();
            if (client->state() != QAbstractSocket::UnconnectedState) {
                client->waitForDisconnected(1000);
            }
            client->deleteLater();
        }
    }
}

void SimpleServer::handleClientData(QTcpSocket *client)
{
    QByteArray data = client->readAll();
    qDebug() << "SimpleServer: Received" << data.size() << "bytes";

    try {
        HttpRequest request = parseHttpRequest(data);
        HttpResponse response = handleRequest(request);
        
        QByteArray responseData = createHttpResponse(response);
        client->write(responseData);
        client->flush();
        
        qDebug() << "SimpleServer: Handled" << request.method << request.path;
        
    } catch (const std::exception &e) {
        qWarning() << "SimpleServer: Error processing request:" << e.what();
        
        HttpResponse errorResponse;
        errorResponse.statusCode = 400;
        errorResponse.headers["Content-Type"] = "application/json";
        errorResponse.body = QString(R"({"error": "Bad Request", "message": "%1"})").arg(e.what()).toUtf8();
        
        client->write(createHttpResponse(errorResponse));
        client->flush();
    }
}

SimpleServer::HttpRequest SimpleServer::parseHttpRequest(const QByteArray &data)
{
    HttpRequest request;
    
    QString requestString = QString::fromUtf8(data);
    QStringList lines = requestString.split("\r\n");
    
    if (lines.isEmpty()) {
        throw std::runtime_error("Empty HTTP request");
    }
    
    // Parse request line
    QStringList requestLine = lines[0].split(" ");
    if (requestLine.size() < 3) {
        throw std::runtime_error("Invalid HTTP request line");
    }
    
    request.method = requestLine[0];
    
    // Parse URL and query string
    QUrl url(requestLine[1]);
    request.path = url.path();
    request.queryString = url.query();
    
    // Parse headers
    int headerEndIndex = 1;
    for (int i = 1; i < lines.size(); ++i) {
        if (lines[i].isEmpty()) {
            headerEndIndex = i;
            break;
        }
        
        QStringList headerParts = lines[i].split(": ");
        if (headerParts.size() >= 2) {
            request.headers[headerParts[0].toLower()] = headerParts.mid(1).join(": ");
        }
    }
    
    // Parse body
    if (headerEndIndex + 1 < lines.size()) {
        QStringList bodyLines = lines.mid(headerEndIndex + 1);
        request.body = bodyLines.join("\r\n").toUtf8();
    }
    
    return request;
}

QByteArray SimpleServer::createHttpResponse(const HttpResponse &response)
{
    QString statusText;
    switch (response.statusCode) {
        case 200: statusText = "OK"; break;
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 500: statusText = "Internal Server Error"; break;
        default: statusText = "Unknown"; break;
    }
    
    QString httpResponse = QString("HTTP/1.1 %1 %2\r\n").arg(response.statusCode).arg(statusText);
    
    // Add default headers
    httpResponse += "Server: GPT4All-Simple/1.0\r\n";
    httpResponse += "Access-Control-Allow-Origin: *\r\n";
    httpResponse += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    httpResponse += "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    
    // Add custom headers
    for (auto it = response.headers.begin(); it != response.headers.end(); ++it) {
        httpResponse += QString("%1: %2\r\n").arg(it.key(), it.value());
    }
    
    httpResponse += QString("Content-Length: %1\r\n").arg(response.body.size());
    httpResponse += "\r\n";
    
    return httpResponse.toUtf8() + response.body;
}

SimpleServer::HttpResponse SimpleServer::handleRequest(const HttpRequest &request)
{
    // Route the request
    if (request.method == "GET" && request.path == "/") {
        return handleHealth(request);
    } else if (request.method == "GET" && request.path == "/v1/models") {
        return handleModels(request);
    } else if (request.method == "POST" && request.path == "/v1/chat/completions") {
        return handleChatCompletions(request);
    } else if (request.method == "OPTIONS") {
        return handleCors(request);
    } else {
        // 404 Not Found
        HttpResponse response;
        response.statusCode = 404;
        response.headers["Content-Type"] = "application/json";
        response.body = R"({"error": "Not Found", "message": "The requested endpoint was not found"})";
        return response;
    }
}

SimpleServer::HttpResponse SimpleServer::handleHealth(const HttpRequest &request)
{
    Q_UNUSED(request);
    
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    
    QJsonObject status;
    status["status"] = "ok";
    status["server"] = "GPT4All-Simple";
    status["version"] = "1.0";
    status["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    if (m_chatLLM) {
        status["model_loaded"] = m_chatLLM->isModelLoaded();
    }
    
    QJsonDocument doc(status);
    response.body = doc.toJson(QJsonDocument::Compact);
    
    return response;
}

SimpleServer::HttpResponse SimpleServer::handleModels(const HttpRequest &request)
{
    Q_UNUSED(request);
    
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    
    QJsonObject result;
    result["object"] = "list";
    
    QJsonArray models;
    QJsonObject model;
    model["id"] = "gpt4all-local";
    model["object"] = "model";
    model["created"] = QDateTime::currentSecsSinceEpoch();
    model["owned_by"] = "gpt4all";
    
    models.append(model);
    result["data"] = models;
    
    QJsonDocument doc(result);
    response.body = doc.toJson(QJsonDocument::Compact);
    
    return response;
}

SimpleServer::HttpResponse SimpleServer::handleChatCompletions(const HttpRequest &request)
{
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    
    if (!m_chatLLM) {
        response.statusCode = 500;
        response.body = R"({"error": "Internal Server Error", "message": "ChatLLM not available"})";
        return response;
    }
    
    if (!m_chatLLM->isModelLoaded()) {
        response.statusCode = 400;
        response.body = R"({"error": "Bad Request", "message": "No model loaded"})";
        return response;
    }
    
    // Parse JSON request
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(request.body, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        response.statusCode = 400;
        response.body = QString(R"({"error": "Bad Request", "message": "Invalid JSON: %1"})")
                           .arg(parseError.errorString()).toUtf8();
        return response;
    }
    
    QJsonObject requestObj = doc.object();
    QJsonArray messages = requestObj["messages"].toArray();
    
    if (messages.isEmpty()) {
        response.statusCode = 400;
        response.body = R"({"error": "Bad Request", "message": "No messages provided"})";
        return response;
    }
    
    // Simple response for now - TODO: integrate with ChatLLM properly
    QJsonObject responseObj;
    responseObj["id"] = QString("chatcmpl-%1").arg(QDateTime::currentMSecsSinceEpoch());
    responseObj["object"] = "chat.completion";
    responseObj["created"] = QDateTime::currentSecsSinceEpoch();
    responseObj["model"] = "gpt4all-local";
    
    QJsonArray choices;
    QJsonObject choice;
    choice["index"] = 0;
    choice["finish_reason"] = "length";
    
    QJsonObject message;
    message["role"] = "assistant";
    message["content"] = "Hello! This is GPT4All local server. The API is working and can receive your requests. Full ChatLLM integration will be added next.";
    
    choice["message"] = message;
    choices.append(choice);
    responseObj["choices"] = choices;
    
    QJsonObject usage;
    usage["prompt_tokens"] = 10;
    usage["completion_tokens"] = 20;
    usage["total_tokens"] = 30;
    responseObj["usage"] = usage;
    
    QJsonDocument responseDoc(responseObj);
    response.body = responseDoc.toJson(QJsonDocument::Compact);
    
    return response;
}

SimpleServer::HttpResponse SimpleServer::handleCors(const HttpRequest &request)
{
    Q_UNUSED(request);
    
    HttpResponse response;
    response.statusCode = 200;
    response.headers["Content-Type"] = "text/plain";
    response.body = "OK";
    
    return response;
}
