#include "qtcphttpserver.h"
#include "chatllm.h"

#include <QTcpSocket>
#include <QNetworkInterface>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QJsonParseError>
#include <QCoreApplication>
#include <QDateTime>

QTcpHttpServer::QTcpHttpServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_chatLLM(nullptr)
{
    connect(m_server, &QTcpServer::newConnection, this, &QTcpHttpServer::onNewConnection);
    setupDefaultRoutes();
}

QTcpHttpServer::~QTcpHttpServer()
{
    close();
}

bool QTcpHttpServer::listen(const QHostAddress &address, quint16 port)
{
    if (m_server->isListening()) {
        close();
    }

    bool success = m_server->listen(address, port);
    if (success) {
        qDebug() << "QTcpHttpServer: Listening on" << address.toString() << "port" << m_server->serverPort();
        emit requestReceived("SERVER", QString("Started on port %1").arg(m_server->serverPort()));
    } else {
        QString error = QString("Failed to start server: %1").arg(m_server->errorString());
        qWarning() << "QTcpHttpServer:" << error;
        emit errorOccurred(error);
    }
    return success;
}

void QTcpHttpServer::close()
{
    if (m_server->isListening()) {
        m_server->close();
        qDebug() << "QTcpHttpServer: Server closed";
    }
}

bool QTcpHttpServer::isListening() const
{
    return m_server->isListening();
}

quint16 QTcpHttpServer::serverPort() const
{
    return m_server->serverPort();
}

void QTcpHttpServer::addRoute(HttpMethod method, const QString &path, RequestHandler handler)
{
    QMutexLocker locker(&m_routesMutex);
    QString key = QString("%1:%2").arg(methodToString(method), path);
    m_routes[key] = handler;
}

void QTcpHttpServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        
        connect(client, &QTcpSocket::readyRead, this, &QTcpHttpServer::onClientDataReady);
        connect(client, &QTcpSocket::disconnected, this, &QTcpHttpServer::onClientDisconnected);
        connect(client, &QTcpSocket::disconnected, client, &QTcpSocket::deleteLater);
        
        qDebug() << "QTcpHttpServer: New client connected from" << client->peerAddress().toString();
    }
}

void QTcpHttpServer::onClientDataReady()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray data = client->readAll();
    qDebug() << "QTcpHttpServer: Received" << data.size() << "bytes";

    try {
        HttpRequest request = parseHttpRequest(data);
        
        // Find matching route
        QMutexLocker locker(&m_routesMutex);
        QString routeKey = QString("%1:%2").arg(methodToString(request.method), request.path);
        
        HttpResponse response;
        if (m_routes.contains(routeKey)) {
            response = m_routes[routeKey](request);
        } else {
            // Default 404 response
            response.statusCode = 404;
            response.headers["Content-Type"] = "application/json";
            response.body = R"({"error": "Not Found", "message": "The requested endpoint was not found"})";
        }

        QByteArray responseData = createHttpResponse(response);
        client->write(responseData);
        client->flush();
        
        if (!response.isStreaming) {
            client->disconnectFromHost();
        }
        
        emit requestReceived(methodToString(request.method), request.path);
        
    } catch (const std::exception &e) {
        qWarning() << "QTcpHttpServer: Error processing request:" << e.what();
        
        HttpResponse errorResponse;
        errorResponse.statusCode = 400;
        errorResponse.headers["Content-Type"] = "application/json";
        errorResponse.body = QString(R"({"error": "Bad Request", "message": "%1"})").arg(e.what()).toUtf8();
        
        client->write(createHttpResponse(errorResponse));
        client->flush();
        client->disconnectFromHost();
    }
}

void QTcpHttpServer::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        qDebug() << "QTcpHttpServer: Client disconnected";
    }
}

QTcpHttpServer::HttpRequest QTcpHttpServer::parseHttpRequest(const QByteArray &data)
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
    
    request.method = stringToMethod(requestLine[0]);
    
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

QByteArray QTcpHttpServer::createHttpResponse(const HttpResponse &response)
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
    httpResponse += "Server: GPT4All-QtTcp/1.0\r\n";
    httpResponse += "Access-Control-Allow-Origin: *\r\n";
    httpResponse += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    httpResponse += "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    
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

QString QTcpHttpServer::methodToString(HttpMethod method) const
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

QTcpHttpServer::HttpMethod QTcpHttpServer::stringToMethod(const QString &method) const
{
    QString upper = method.toUpper();
    if (upper == "GET") return GET;
    if (upper == "POST") return POST;
    if (upper == "PUT") return PUT;
    if (upper == "DELETE") return DELETE;
    if (upper == "OPTIONS") return OPTIONS;
    return GET; // Default
}

void QTcpHttpServer::setupDefaultRoutes()
{
    // Health check endpoint
    addRoute(GET, "/", [this](const HttpRequest &request) -> HttpResponse {
        return handleHealth(request);
    });
    
    // OpenAI-compatible models endpoint
    addRoute(GET, "/v1/models", [this](const HttpRequest &request) -> HttpResponse {
        return handleModels(request);
    });
    
    // OpenAI-compatible chat completions endpoint
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

QTcpHttpServer::HttpResponse QTcpHttpServer::handleHealth(const HttpRequest &request)
{
    Q_UNUSED(request);
    
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    
    QJsonObject status;
    status["status"] = "ok";
    status["server"] = "GPT4All-QtTcp";
    status["version"] = "1.0";
    status["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    if (m_chatLLM) {
        status["model_loaded"] = m_chatLLM->isModelLoaded();
    }
    
    QJsonDocument doc(status);
    response.body = doc.toJson(QJsonDocument::Compact);
    
    return response;
}

QTcpHttpServer::HttpResponse QTcpHttpServer::handleModels(const HttpRequest &request)
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

QTcpHttpServer::HttpResponse QTcpHttpServer::handleChatCompletions(const HttpRequest &request)
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
    
    // Convert messages to prompt format
    QString prompt = "<chat>\n";
    for (const QJsonValue &value : messages) {
        QJsonObject message = value.toObject();
        QString role = message["role"].toString();
        QString content = message["content"].toString();
        
        if (role == "system") {
            prompt += QString("<system>%1</system>\n").arg(content);
        } else if (role == "user") {
            prompt += QString("<user>%1</user>\n").arg(content);
        } else if (role == "assistant") {
            prompt += QString("<assistant>%1</assistant>\n").arg(content);
        }
    }
    prompt += "</chat>";
    
    // Simple non-streaming response for now
    // TODO: Implement proper streaming support
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
    message["content"] = "This is a test response from GPT4All local server. Full integration with ChatLLM is in progress.";
    
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

QTcpHttpServer::HttpResponse QTcpHttpServer::handleCors(const HttpRequest &request)
{
    Q_UNUSED(request);
    
    HttpResponse response;
    response.statusCode = 200;
    response.headers["Content-Type"] = "text/plain";
    response.body = "OK";
    
    return response;
}
