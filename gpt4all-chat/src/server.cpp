// Qt 6.2 compatibility stub implementation for server.cpp
// QHttpServer not available in Qt 6.2, so HTTP server functionality is disabled

#include "server.h"
#include "chat.h"
#include "chatapi.h"
#include "chatllm.h"
#include "database.h"
#include "localdocs.h"
#include "mysettings.h"

#include <QDebug>
#include <QJsonObject>

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
    // Qt 6.2 compatibility - QHttpServer not available
    qDebug() << "Server created but HTTP functionality disabled (Qt 6.2 compatibility)";
    
    connect(this, &Server::threadStarted, this, &Server::start);
    
    // Qt 6.2 compatibility - database connections disabled since HTTP server is not functional
}

void Server::start()
{
    // Qt 6.2 compatibility - QHttpServer not available, HTTP server functionality disabled
    qDebug() << "Server::start() - HTTP server functionality not available in Qt 6.2";

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
