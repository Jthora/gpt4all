#ifndef ENHANCED_AI_SERVER_FIXED_H
#define ENHANCED_AI_SERVER_FIXED_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include <QHash>
#include <QMutex>

// GPT4All includes
#include "chatllm.h"
#include "chatmodel.h"
#include "modellist.h"
#include "database.h"
#include "chat.h"

struct AISession {
    QString sessionId;
    QPointer<QTcpSocket> socket;
    QString accumulatedResponse;
    bool isStreaming;
    QJsonObject originalRequest;
    QString currentModel;
    QTimer* timeoutTimer;
    
    AISession() : isStreaming(false), timeoutTimer(nullptr) {}
};

class EnhancedAIServerFixed : public QObject
{
    Q_OBJECT
    
public:
    explicit EnhancedAIServerFixed(QObject *parent = nullptr);
    ~EnhancedAIServerFixed();
    
    bool startServer(quint16 port = 4891);
    void stopServer();
    bool isRunning() const;

private slots:
    void handleNewConnection();
    void handleClientDisconnected();
    void handleReadyRead();
    void handleSessionTimeout();
    
    // ChatLLM integration - CORRECTED
    void handleResponseChanged();
    void handleResponseFinished();
    void handleResponseFailed();

private:
    // HTTP handling
    void processHttpRequest(QTcpSocket* socket, const QByteArray& requestData);
    void handleChatCompletions(QTcpSocket* socket, const QJsonObject& request);
    void handleModelsRequest(QTcpSocket* socket);
    
    // Response utilities
    QByteArray createHttpResponse(const QJsonObject& jsonResponse, 
                                 const QString& contentType = "application/json",
                                 int statusCode = 200) const;
    QJsonObject createErrorResponse(const QString& message, const QString& type = "invalid_request") const;
    QJsonObject createStreamingChunk(const QString& sessionId, const QString& content, bool isEnd = false) const;
    
    // AI Integration - CORRECTED
    bool setupAIChat(const QJsonArray& messages, const QString& model);
    void startAIGeneration(const QString& sessionId, const QJsonObject& request);
    void cleanupSession(const QString& sessionId);
    
    // Model management
    bool loadModelIfNeeded(const QString& modelName);
    QStringList getAvailableModels() const;
    
    // Session management
    QString generateSessionId() const;
    AISession* findSessionBySocket(QTcpSocket* socket);
    
private:
    QTcpServer* m_server;
    QHash<QString, AISession*> m_sessions;
    QMutex m_sessionsMutex;
    
    // GPT4All components - CORRECTED INITIALIZATION
    Chat* m_chat;
    ChatLLM* m_chatLLM;
    ChatModel* m_chatModel;
    ModelList* m_modelList;
    Database* m_database;
    
    // Current session tracking
    QString m_currentSessionId;
    bool m_generationInProgress;
    
    static const int SESSION_TIMEOUT_MS = 30000; // 30 seconds
};

#endif // ENHANCED_AI_SERVER_FIXED_H
