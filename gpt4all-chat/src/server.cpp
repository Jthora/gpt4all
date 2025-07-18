// Qt 6.2 compatibility implementation using SimpleTcpServer instead of QHttpServer
// Provides OpenAI-compatible API server functionality

#include "server.h"
#include "chat.h"
#include "chatapi.h"
#include "chatllm.h"
#include "database.h"
#include "localdocs.h"
#include "mysettings.h"
#include "simpleserver.h"

#include <QDebug>
#include <QJsonObject>
#include <QHostAddress>
#include <QTimer>

// Stub classes for request types
class CompletionRequest {
public:
    static CompletionRequest fromJson(const QByteArray &) { return CompletionRequest(); }
};

class ChatRequest {
public:
    static ChatRequest fromJson(const QByteArray &) { return ChatRequest(); }
};

Server::Server(Chat *chat)
    : ChatLLM(chat, true /* is server */)
    , m_chat(chat)
{
    // Initialize SimpleServer instead of QHttpServer
    m_httpServer = new SimpleServer();
    m_httpServer->setChatLLM(this);
    
    qDebug() << "Server created with SimpleServer support";
    
    connect(this, &Server::threadStarted, this, &Server::start);
}

void Server::start()
{
    // Check if server is enabled in settings
    if (!MySettings::globalInstance()->serverChat()) {
        qDebug() << "Server::start() - API server is disabled in settings";
        return;
    }
    
    // Start the SimpleServer on the configured port
    int port = MySettings::globalInstance()->networkPort();
    
    bool success = m_httpServer->startServer(port);
    if (success) {
        qDebug() << "Server::start() - HTTP server started successfully on port" << port;
    } else {
        qWarning() << "Server::start() - Failed to start HTTP server on port" << port;
    }

    connect(this, &Server::requestResetResponseState, m_chat, &Chat::resetResponseState, Qt::BlockingQueuedConnection);
}

auto Server::handleCompletionRequest(const CompletionRequest &request) -> std::pair<HttpServerResponse, std::optional<QJsonObject>>
{
    Q_UNUSED(request);
    // Qt 6.2 compatibility - return stub response
    return { HttpServerResponse(500), std::nullopt };
}

auto Server::handleChatRequest(const ChatRequest &request) -> std::pair<HttpServerResponse, std::optional<QJsonObject>>
{
    Q_UNUSED(request);
    // Qt 6.2 compatibility - return stub response  
    return { HttpServerResponse(500), std::nullopt };
}
