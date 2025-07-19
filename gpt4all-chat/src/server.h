#ifndef SERVER_H
#define SERVER_H

#include "chatllm.h"
#include "database.h"

// Qt 6.2 compatibility - QHttpServer not available, create stub implementation
#include <QJsonObject>
#include <QList>
#include <QObject> // IWYU pragma: keep
#include <QString>

#include <memory>
#include <optional>
#include <utility>

class Chat;
class ChatRequest;
class CompletionRequest;

// Qt 6.2 compatibility stub for QHttpServerResponse
struct HttpServerResponse {
    QJsonObject data;
    int statusCode = 200;
    HttpServerResponse() = default;
    HttpServerResponse(const QJsonObject &obj) : data(obj) {}
    HttpServerResponse(int code) : statusCode(code) {}
};

class Server : public ChatLLM
{
    Q_OBJECT

public:
    explicit Server(Chat *chat);
    ~Server() override = default;

public Q_SLOTS:
    void start();

Q_SIGNALS:
    void requestResetResponseState();

private:
    auto handleCompletionRequest(const CompletionRequest &request) -> std::pair<HttpServerResponse, std::optional<QJsonObject>>;
    auto handleChatRequest(const ChatRequest &request) -> std::pair<HttpServerResponse, std::optional<QJsonObject>>;

private Q_SLOTS:
    void handleDatabaseResultsChanged(const QList<ResultInfo> &results) { m_databaseResults = results; }
    void handleCollectionListChanged(const QList<QString> &collectionList) { m_collections = collectionList; }

private:
    Chat *m_chat;
    // QHttpServer not available in Qt 6.2, HTTP server functionality disabled
    QList<ResultInfo> m_databaseResults;
    QList<QString> m_collections;
};

#endif // SERVER_H
