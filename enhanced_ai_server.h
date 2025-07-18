// =============================================================================
// COMPLETE AI-ENHANCED SERVER IMPLEMENTATION
// Enhanced QTcpHttpServer with full GPT4All ChatLLM integration
// =============================================================================

#ifndef ENHANCED_AI_SERVER_H
#define ENHANCED_AI_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QHostAddress>
#include <QDateTime>
#include <QUuid>
#include <QMap>
#include <functional>

// GPT4All AI Components
class ChatLLM;
class ModelList;
class Database;
class Chat;

class EnhancedAIServer : public QObject
{
    Q_OBJECT

public:
    enum HttpMethod {
        GET,
        POST,
        PUT,
        DELETE,
        OPTIONS
    };

    struct HttpRequest {
        HttpMethod method;
        QString path;
        QMap<QString, QString> headers;
        QByteArray body;
        QString queryString;
    };

    struct HttpResponse {
        int statusCode = 200;
        QMap<QString, QString> headers;
        QByteArray body;
        bool isStreaming = false;
    };

    // Generation session tracking
    struct GenerationSession {
        QString sessionId;
        QString requestId;
        QTcpSocket *socket;
        QJsonObject originalRequest;
        QString accumulatedResponse;
        bool isStreaming;
        bool isComplete;
        QDateTime startTime;
    };

    using RequestHandler = std::function<HttpResponse(const HttpRequest&)>;

    explicit EnhancedAIServer(QObject *parent = nullptr);
    ~EnhancedAIServer();

    // Server control
    bool listen(const QHostAddress &address = QHostAddress::LocalHost, quint16 port = 4891);
    void close();
    bool isListening() const;
    quint16 serverPort() const;

    // AI Integration - THE CRITICAL DIFFERENCE
    void setChatLLM(ChatLLM *chatLLM);
    void setModelList(ModelList *modelList);
    void setDatabase(Database *database);
    void setChat(Chat *chat);

    // Route management
    void addRoute(HttpMethod method, const QString &path, RequestHandler handler);

signals:
    void requestReceived(const QString &method, const QString &path);
    void errorOccurred(const QString &error);
    void aiResponseGenerated(const QString &sessionId, const QString &response);

private slots:
    // HTTP handling
    void onNewConnection();
    void onClientDataReady();
    void onClientDisconnected();
    
    // REAL AI Response handling - THE KEY INTEGRATION
    void handleAIResponseChanged();
    void handleAIResponseStopped(qint64 promptResponseMs);
    void handleModelLoadingPercentageChanged(float percentage);
    void handleModelLoadingError(const QString &error);

private:
    // HTTP Server components
    QTcpServer *m_server;
    QMap<QString, RequestHandler> m_routes;
    QMutex m_routesMutex;

    // AI Components - REAL INTEGRATION
    ChatLLM *m_chatLLM;
    ModelList *m_modelList;
    Database *m_database;
    Chat *m_chat;

    // Active generation tracking
    QMap<QString, GenerationSession> m_activeSessions;
    QMap<QTcpSocket*, QString> m_socketToSession;
    QMutex m_sessionMutex;

    // HTTP processing
    HttpRequest parseHttpRequest(const QByteArray &data);
    QByteArray createHttpResponse(const HttpResponse &response);
    QString methodToString(HttpMethod method) const;
    HttpMethod stringToMethod(const QString &method) const;
    
    // Built-in handlers for OpenAI-compatible API
    HttpResponse handleModels(const HttpRequest &request);
    HttpResponse handleChatCompletions(const HttpRequest &request);
    HttpResponse handleCompletions(const HttpRequest &request);
    HttpResponse handleHealth(const HttpRequest &request);
    HttpResponse handleCors(const HttpRequest &request);
    
    // REAL AI methods - THE CORE FUNCTIONALITY
    QString startAIGeneration(const QJsonObject &request, QTcpSocket *socket = nullptr);
    bool loadModelIfNeeded(const QString &modelName);
    void setupChatForPrompt(const QJsonArray &messages);
    QString buildPromptFromMessages(const QJsonArray &messages);
    QJsonObject createCompletionResponse(const QString &sessionId, const QString &response, bool isChat = true);
    QJsonObject createStreamingChunk(const QString &sessionId, const QString &deltaContent, bool isComplete = false);
    
    // Session management
    QString createGenerationSession(const QJsonObject &request, QTcpSocket *socket, bool streaming);
    void completeGenerationSession(const QString &sessionId);
    void cleanupSession(const QString &sessionId);
    
    // Error handling
    QJsonObject createErrorResponse(const QString &type, const QString &message);
    
    void setupDefaultRoutes();
};

#endif // ENHANCED_AI_SERVER_H
