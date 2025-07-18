#ifndef REAL_AI_SIMPLE_SERVER_H
#define REAL_AI_SIMPLE_SERVER_H

#include "minimal_simpleserver.h"
#include "chatllm.h"
#include "modellist.h"
#include "database.h"
#include "mysettings.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QHostAddress>
#include <QDebug>
#include <QDateTime>
#include <QThread>
#include <QTimer>

class RealAISimpleServer : public QObject
{
    Q_OBJECT

public:
    // Inherit HTTP structures from MinimalSimpleServer
    using HttpRequest = MinimalSimpleServer::HttpRequest;
    using HttpResponse = MinimalSimpleServer::HttpResponse;

    explicit RealAISimpleServer(QObject *parent = nullptr);
    ~RealAISimpleServer();

    // Core server methods
    bool startServer(quint16 port = 4891);
    void stopServer();
    bool isListening() const;

    // AI Integration
    void setChatLLM(ChatLLM *chatLLM);
    void setModelList(ModelList *modelList);
    void setDatabase(Database *database);

private slots:
    // HTTP handling
    void handleNewConnection();
    void handleClientData();
    
    // AI Response handling
    void handleResponseChanged();
    void handleResponseStopped();
    void handleModelLoadingPercentageChanged(float percentage);
    void handleModelLoadingError(const QString &error);

private:
    // HTTP Server components
    QTcpServer *m_server;
    QMap<QTcpSocket*, HttpRequest> m_activeRequests;
    
    // AI Components - The real GPT4All integration
    ChatLLM *m_chatLLM;
    ModelList *m_modelList; 
    Database *m_database;
    
    // Current generation state
    struct GenerationState {
        QTcpSocket *socket;
        QString requestId;
        QJsonObject originalRequest;
        QString accumulatedResponse;
        bool isStreaming;
        QDateTime startTime;
    };
    
    QMap<QString, GenerationState> m_activeGenerations;
    
    // HTTP processing
    HttpRequest parseHttpRequest(const QByteArray &data);
    QByteArray createHttpResponse(const HttpResponse &response);
    HttpResponse handleRequest(const HttpRequest &request, QTcpSocket *socket);
    
    // API endpoints with real AI
    HttpResponse handleHealthCheck();
    HttpResponse handleModels();
    HttpResponse handleChatCompletions(const HttpRequest &request, QTcpSocket *socket);
    HttpResponse handleCompletions(const HttpRequest &request, QTcpSocket *socket);
    
    // Real AI methods
    QString generateAIResponse(const QJsonObject &request);
    void startStreamingGeneration(const QJsonObject &request, QTcpSocket *socket);
    bool loadModelIfNeeded(const QString &modelName);
    QStringList getAvailableModels();
    
    // Validation
    bool validateChatRequest(const QJsonObject &request);
    bool validateCompletionRequest(const QJsonObject &request);
    
    // Streaming support
    void sendStreamingChunk(QTcpSocket *socket, const QJsonObject &chunk);
    void finishStreamingResponse(QTcpSocket *socket);
    
    // Utilities
    QJsonObject createErrorResponse(const QString &error, const QString &message);
    QString generateRequestId();
};

#endif // REAL_AI_SIMPLE_SERVER_H
