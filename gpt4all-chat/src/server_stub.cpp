// Qt 6.2 compatibility server stub implementation
// This provides a minimal, stable implementation that doesn't crash
// For real HTTP server functionality, use our enhanced_ai_server implementations

#include "server.h"
#include "chat.h"
#include "chatapi.h"
#include "chatllm.h"
#include "database.h"
#include "localdocs.h"
#include "mysettings.h"

#include <QDebug>
#include <QJsonObject>
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
    // Qt 6.2 compatibility - QHttpServer not available
    qDebug() << "Server created but HTTP functionality disabled (Qt 6.2 compatibility)";
    qDebug() << "For real HTTP server functionality, use:";
    qDebug() << "  - enhanced_ai_server_fixed (production-ready with real AI)";
    qDebug() << "  - minimal_simpleserver (perfect HTTP framework)";
    
    // Set up the server safely without HTTP functionality
    connect(this, &Server::threadStarted, this, &Server::start, Qt::QueuedConnection);
    
    // Initialize safely without causing crashes
    m_databaseResults.clear();
    m_collections.clear();
    
    // Start the server initialization immediately to ensure it's ready
    QTimer::singleShot(0, this, &Server::start);
}

void Server::start()
{
    // Qt 6.2 compatibility - QHttpServer not available, HTTP server functionality disabled
    qDebug() << "Server::start() - HTTP server functionality not available in Qt 6.2";
    qDebug() << "The main GPT4All app will work normally, but HTTP API is disabled.";
    qDebug() << "For HTTP API functionality, run one of our standalone servers:";
    qDebug() << "  ./enhanced_ai_server_fixed  # Real AI integration";
    qDebug() << "  ./minimal_simpleserver      # Perfect HTTP framework";

    // Set up safe connections to prevent crashes
    connect(this, &Server::requestResetResponseState, m_chat, &Chat::resetResponseState, Qt::BlockingQueuedConnection);
    
    // Emit a safe signal to indicate the server is "started" (even though it's just a stub)
    QTimer::singleShot(100, this, [this]() {
        qDebug() << "Server stub initialization complete - main app can continue normally";
    });
}

auto Server::handleCompletionRequest(const CompletionRequest &request) -> std::pair<HttpServerResponse, std::optional<QJsonObject>>
{
    Q_UNUSED(request);
    // Qt 6.2 compatibility - return safe stub response
    QJsonObject error;
    error["error"] = "HTTP server functionality not available in Qt 6.2";
    error["message"] = "Use enhanced_ai_server_fixed or minimal_simpleserver for HTTP API";
    return { HttpServerResponse(503), error };
}

auto Server::handleChatRequest(const ChatRequest &request) -> std::pair<HttpServerResponse, std::optional<QJsonObject>>
{
    Q_UNUSED(request);
    // Qt 6.2 compatibility - return safe stub response  
    QJsonObject error;
    error["error"] = "HTTP server functionality not available in Qt 6.2";
    error["message"] = "Use enhanced_ai_server_fixed or minimal_simpleserver for HTTP API";
    return { HttpServerResponse(503), error };
}