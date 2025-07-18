#include "simpletcpserver.h"
#include "chatllm.h"

#include <QTcpSocket>
#include <QUrl>
#include <QJsonParseError>

SimpleTcpServer::SimpleTcpServer(QObject *parent)
    : QTcpServer(parent)
    , m_chatLLM(nullptr)
{
}

SimpleTcpServer::~SimpleTcpServer()
{
    close();
}

bool SimpleTcpServer::startServer(quint16 port)
{
    if (isListening()) {
        close();
    }

    bool success = listen(QHostAddress::LocalHost, port);
    if (success) {
        qDebug() << "SimpleTcpServer: HTTP server started on port" << serverPort();
        qDebug() << "OpenAI-compatible API available at: http://localhost:" << serverPort();
        qDebug() << "  Health check: http://localhost:" << serverPort() << "/";
        qDebug() << "  Models: http://localhost:" << serverPort() << "/v1/models";
        qDebug() << "  Chat completions: http://localhost:" << serverPort() << "/v1/chat/completions";
    } else {
        qWarning() << "SimpleTcpServer: Failed to start server on port" << port << "-" << errorString();
    }
    return success;
}

void SimpleTcpServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *client = new QTcpSocket(this);
    if (client->setSocketDescriptor(socketDescriptor)) {
        connect(client, &QTcpSocket::readyRead, this, &SimpleTcpServer::onClientDataReady);
        connect(client, &QTcpSocket::disconnected, this, &SimpleTcpServer::onClientDisconnected);
        connect(client, &QTcpSocket::disconnected, client, &QTcpSocket::deleteLater);
        
        qDebug() << "SimpleTcpServer: New client connected from" << client->peerAddress().toString();
    } else {
        delete client;
    }
}

void SimpleTcpServer::onClientDataReady()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray data = client->readAll();
    qDebug() << "SimpleTcpServer: Received" << data.size() << "bytes";

    try {
        HttpRequest request = parseHttpRequest(data);
        HttpResponse response = handleRequest(request);
        
        QByteArray responseData = createHttpResponse(response);
        client->write(responseData);
        client->flush();
        
        client->disconnectFromHost();
        
        qDebug() << "SimpleTcpServer: Handled" << request.method << request.path;
        
    } catch (const std::exception &e) {
        qWarning() << "SimpleTcpServer: Error processing request:" << e.what();
        
        HttpResponse errorResponse;
        errorResponse.statusCode = 400;
        errorResponse.headers["Content-Type"] = "application/json";
        errorResponse.body = QString(R"({"error": "Bad Request", "message": "%1"})").arg(e.what()).toUtf8();
        
        client->write(createHttpResponse(errorResponse));
        client->flush();
        client->disconnectFromHost();
    }
}

void SimpleTcpServer::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        qDebug() << "SimpleTcpServer: Client disconnected";
    }
}

SimpleTcpServer::HttpRequest SimpleTcpServer::parseHttpRequest(const QByteArray &data)
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

QByteArray SimpleTcpServer::createHttpResponse(const HttpResponse &response)
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
    httpResponse += "Server: GPT4All-SimpleTcp/1.0\r\n";
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

SimpleTcpServer::HttpResponse SimpleTcpServer::handleRequest(const HttpRequest &request)
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

SimpleTcpServer::HttpResponse SimpleTcpServer::handleHealth(const HttpRequest &request)
{
    Q_UNUSED(request);
    
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    
    QJsonObject status;
    status["status"] = "ok";
    status["server"] = "GPT4All-SimpleTcp";
    status["version"] = "1.0";
    status["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    if (m_chatLLM) {
        status["model_loaded"] = m_chatLLM->isModelLoaded();
    }
    
    QJsonDocument doc(status);
    response.body = doc.toJson(QJsonDocument::Compact);
    
    return response;
}

SimpleTcpServer::HttpResponse SimpleTcpServer::handleModels(const HttpRequest &request)
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

SimpleTcpServer::HttpResponse SimpleTcpServer::handleChatCompletions(const HttpRequest &request)
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
    message["content"] = "Hello! This is GPT4All local server. The API is working, but full ChatLLM integration is still in development.";
    
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

SimpleTcpServer::HttpResponse SimpleTcpServer::handleCors(const HttpRequest &request)
{
    Q_UNUSED(request);
    
    HttpResponse response;
    response.statusCode = 200;
    response.headers["Content-Type"] = "text/plain";
    response.body = "OK";
    
    return response;
}
