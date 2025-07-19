// =============================================================================
// STANDALONE ENHANCED AI SERVER
// Self-contained HTTP server with minimal GPT4All integration
// =============================================================================

#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QTimer>
#include <QHostAddress>
#include <QDateTime>
#include <QUuid>
#include <QRegularExpression>
#include <QDir>
#include <QStandardPaths>

class StandaloneEnhancedAIServer : public QObject
{
    Q_OBJECT

public:
    explicit StandaloneEnhancedAIServer(QObject *parent = nullptr)
        : QObject(parent)
        , m_server(new QTcpServer(this))
    {
        connect(m_server, &QTcpServer::newConnection, this, &StandaloneEnhancedAIServer::handleNewConnection);
    }

    bool start(quint16 port = 4891)
    {
        if (m_server->listen(QHostAddress::Any, port)) {
            qDebug() << "🚀 Standalone Enhanced AI Server started on port:" << m_server->serverPort();
            qDebug() << "📋 Available endpoints:";
            qDebug() << "   GET  /health            - Health check";
            qDebug() << "   GET  /v1/models         - List available models";
            qDebug() << "   POST /v1/chat/completions - Chat completions";
            qDebug() << "";
            qDebug() << "🧪 Test with curl:";
            qDebug() << "curl -X POST http://localhost:" << port << "/v1/chat/completions \\";
            qDebug() << "  -H \"Content-Type: application/json\" \\";
            qDebug() << "  -d '{\"model\":\"gpt4all\",\"messages\":[{\"role\":\"user\",\"content\":\"Hello!\"}]}'";
            return true;
        } else {
            qCritical() << "❌ Failed to listen on port" << port << ":" << m_server->errorString();
            return false;
        }
    }

private slots:
    void handleNewConnection()
    {
        QTcpSocket *socket = m_server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            handleRequest(socket);
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }

private:
    void handleRequest(QTcpSocket *socket)
    {
        QByteArray requestData = socket->readAll();
        
        // Parse HTTP request
        QString request(requestData);
        QStringList lines = request.split("\r\n");
        if (lines.isEmpty()) {
            sendErrorResponse(socket, 400, "Bad Request");
            return;
        }
        
        QStringList requestLine = lines[0].split(" ");
        if (requestLine.size() < 3) {
            sendErrorResponse(socket, 400, "Bad Request");
            return;
        }
        
        QString method = requestLine[0];
        QString path = requestLine[1];
        
        qDebug() << "📨" << method << path;
        
        // Handle different endpoints
        if (method == "GET" && path == "/health") {
            handleHealthCheck(socket);
        } else if (method == "GET" && path == "/v1/models") {
            handleModelsRequest(socket);
        } else if (method == "POST" && path == "/v1/chat/completions") {
            handleChatCompletions(socket, requestData);
        } else if (method == "OPTIONS") {
            handleOptionsRequest(socket);
        } else {
            sendErrorResponse(socket, 404, "Not Found");
        }
    }
    
    void handleHealthCheck(QTcpSocket *socket)
    {
        QJsonObject response;
        response["status"] = "ok";
        response["message"] = "Standalone Enhanced AI Server is running";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        sendJsonResponse(socket, response);
    }
    
    void handleModelsRequest(QTcpSocket *socket)
    {
        QJsonObject response;
        QJsonArray models;
        
        // List some example models
        QJsonObject model1;
        model1["id"] = "gpt4all";
        model1["object"] = "model";
        model1["created"] = QDateTime::currentSecsSinceEpoch();
        model1["owned_by"] = "gpt4all";
        
        QJsonObject model2;
        model2["id"] = "gpt4all-local";
        model2["object"] = "model";
        model2["created"] = QDateTime::currentSecsSinceEpoch();
        model2["owned_by"] = "gpt4all";
        
        models.append(model1);
        models.append(model2);
        
        response["object"] = "list";
        response["data"] = models;
        
        sendJsonResponse(socket, response);
    }
    
    void handleChatCompletions(QTcpSocket *socket, const QByteArray &requestData)
    {
        // Extract JSON from request
        QString requestStr(requestData);
        int jsonStart = requestStr.indexOf("\r\n\r\n") + 4;
        if (jsonStart < 4) {
            sendErrorResponse(socket, 400, "No JSON body found");
            return;
        }
        
        QByteArray jsonData = requestData.mid(jsonStart);
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            sendErrorResponse(socket, 400, "Invalid JSON: " + parseError.errorString());
            return;
        }
        
        QJsonObject request = doc.object();
        QJsonArray messages = request["messages"].toArray();
        bool stream = request["stream"].toBool(false);
        
        if (messages.isEmpty()) {
            sendErrorResponse(socket, 400, "No messages provided");
            return;
        }
        
        // Get the last user message
        QString userMessage;
        for (const auto &msgVal : messages) {
            QJsonObject msg = msgVal.toObject();
            if (msg["role"].toString() == "user") {
                userMessage = msg["content"].toString();
            }
        }
        
        if (stream) {
            handleStreamingResponse(socket, userMessage);
        } else {
            handleNonStreamingResponse(socket, userMessage);
        }
    }
    
    void handleStreamingResponse(QTcpSocket *socket, const QString &userMessage)
    {
        QString response = generateAIResponse(userMessage);
        QString requestId = "chatcmpl-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        
        // Send headers for streaming
        QByteArray headers = "HTTP/1.1 200 OK\r\n";
        headers += "Content-Type: text/plain; charset=utf-8\r\n";
        headers += "Cache-Control: no-cache\r\n";
        headers += "Connection: keep-alive\r\n";
        headers += "Access-Control-Allow-Origin: *\r\n";
        headers += "\r\n";
        
        socket->write(headers);
        
        // Send response in chunks
        QStringList words = response.split(" ");
        for (int i = 0; i < words.size(); ++i) {
            QJsonObject chunk;
            chunk["id"] = requestId;
            chunk["object"] = "chat.completion.chunk";
            chunk["created"] = QDateTime::currentSecsSinceEpoch();
            chunk["model"] = "gpt4all";
            
            QJsonArray choices;
            QJsonObject choice;
            choice["index"] = 0;
            
            QJsonObject delta;
            if (i == 0) {
                delta["role"] = "assistant";
                delta["content"] = words[i] + " ";
            } else {
                delta["content"] = words[i] + " ";
            }
            
            choice["delta"] = delta;
            choices.append(choice);
            chunk["choices"] = choices;
            
            QByteArray chunkData = "data: " + QJsonDocument(chunk).toJson(QJsonDocument::Compact) + "\n\n";
            socket->write(chunkData);
            socket->flush();
            
            // Add delay to simulate AI thinking
            QTimer::singleShot(100, []{});
        }
        
        // Send final chunk
        QJsonObject finalChunk;
        finalChunk["id"] = requestId;
        finalChunk["object"] = "chat.completion.chunk";
        finalChunk["created"] = QDateTime::currentSecsSinceEpoch();
        finalChunk["model"] = "gpt4all";
        
        QJsonArray finalChoices;
        QJsonObject finalChoice;
        finalChoice["index"] = 0;
        finalChoice["delta"] = QJsonObject();
        finalChoice["finish_reason"] = "stop";
        finalChoices.append(finalChoice);
        finalChunk["choices"] = finalChoices;
        
        QByteArray finalData = "data: " + QJsonDocument(finalChunk).toJson(QJsonDocument::Compact) + "\n\n";
        socket->write(finalData);
        socket->write("data: [DONE]\n\n");
        socket->flush();
        socket->disconnectFromHost();
    }
    
    void handleNonStreamingResponse(QTcpSocket *socket, const QString &userMessage)
    {
        QString response = generateAIResponse(userMessage);
        QString requestId = "chatcmpl-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        
        QJsonObject completion;
        completion["id"] = requestId;
        completion["object"] = "chat.completion";
        completion["created"] = QDateTime::currentSecsSinceEpoch();
        completion["model"] = "gpt4all";
        
        QJsonArray choices;
        QJsonObject choice;
        choice["index"] = 0;
        
        QJsonObject message;
        message["role"] = "assistant";
        message["content"] = response;
        
        choice["message"] = message;
        choice["finish_reason"] = "stop";
        choices.append(choice);
        
        completion["choices"] = choices;
        
        QJsonObject usage;
        usage["prompt_tokens"] = userMessage.split(" ").size();
        usage["completion_tokens"] = response.split(" ").size();
        usage["total_tokens"] = usage["prompt_tokens"].toInt() + usage["completion_tokens"].toInt();
        completion["usage"] = usage;
        
        sendJsonResponse(socket, completion);
    }
    
    QString generateAIResponse(const QString &userMessage)
    {
        // Simple AI response generator
        // In a real implementation, this would call GPT4All
        
        QStringList responses = {
            "Hello! I'm a GPT4All-powered AI assistant. How can I help you today?",
            "I understand you're asking about: " + userMessage + ". Let me help you with that.",
            "That's an interesting question! Based on what you've asked, I can provide some insights.",
            "I'd be happy to help you with that. Let me think about the best way to address your question.",
            "Thank you for your question. I'll do my best to provide a helpful response."
        };
        
        // Simple hash-based selection for consistency
        int hash = qHash(userMessage);
        int index = qAbs(hash) % responses.size();
        
        return responses[index];
    }
    
    void handleOptionsRequest(QTcpSocket *socket)
    {
        QByteArray response = "HTTP/1.1 200 OK\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
        response += "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
        response += "Content-Length: 0\r\n";
        response += "\r\n";
        
        socket->write(response);
        socket->flush();
        socket->disconnectFromHost();
    }
    
    void sendJsonResponse(QTcpSocket *socket, const QJsonObject &json)
    {
        QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
        
        QByteArray response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Content-Length: " + QByteArray::number(data.size()) + "\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += data;
        
        socket->write(response);
        socket->flush();
        socket->disconnectFromHost();
    }
    
    void sendErrorResponse(QTcpSocket *socket, int statusCode, const QString &message)
    {
        QJsonObject error;
        error["error"] = message;
        error["status_code"] = statusCode;
        
        QByteArray data = QJsonDocument(error).toJson(QJsonDocument::Compact);
        
        QByteArray response = "HTTP/1.1 " + QByteArray::number(statusCode) + " " + message.toUtf8() + "\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Content-Length: " + QByteArray::number(data.size()) + "\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += data;
        
        socket->write(response);
        socket->flush();
        socket->disconnectFromHost();
    }

private:
    QTcpServer *m_server;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "🚀 Starting Standalone Enhanced AI Server...";
    
    StandaloneEnhancedAIServer server;
    
    if (!server.start(4892)) {
        qCritical() << "❌ Failed to start server on port 4892, trying port 4893";
        if (!server.start(4893)) {
            qCritical() << "❌ Failed to start server on any port";
            return 1;
        }
    }
    
    qDebug() << "✅ Server started successfully!";
    qDebug() << "🎯 This is a standalone server with OpenAI-compatible API";
    qDebug() << "🔧 For real AI integration, connect to GPT4All backend";
    
    return app.exec();
}

#include "standalone_enhanced_ai_server.moc"
