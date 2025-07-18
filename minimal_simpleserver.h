#ifndef MINIMAL_SIMPLESERVER_H
#define MINIMAL_SIMPLESERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QHostAddress>
#include <QDebug>
#include <QDateTime>

// Minimal HTTP server for testing
class MinimalSimpleServer
{
public:
    struct HttpRequest {
        QString method;
        QString path;
        QMap<QString, QString> headers;
        QByteArray body;
        QString queryString;
    };

    struct HttpResponse {
        int statusCode = 200;
        QMap<QString, QString> headers;
        QByteArray body;
    };

    explicit MinimalSimpleServer();
    ~MinimalSimpleServer();

    bool startServer(quint16 port = 4891);
    void stopServer();
    bool isListening() const;

private:
    QTcpServer *m_server;
    
    void handleNewConnection();
    void handleClientData(QTcpSocket *client);
    
    HttpRequest parseHttpRequest(const QByteArray &data);
    QByteArray createHttpResponse(const HttpResponse &response);
    HttpResponse handleRequest(const HttpRequest &request);
    
    // API endpoints
    HttpResponse handleHealthCheck();
    HttpResponse handleModels();
    HttpResponse handleChatCompletions(const HttpRequest &request);
    HttpResponse handleCompletions(const HttpRequest &request);
    HttpResponse handleStreamingResponse(const QJsonObject &request, bool isChat = true);
    
    // Helper methods
    QJsonObject generateChatResponse(const QJsonObject &request);
    QJsonObject generateCompletionResponse(const QJsonObject &request);
    QString generateResponseText(const QString &prompt, const QString &model);
    bool validateChatRequest(const QJsonObject &request);
    bool validateCompletionRequest(const QJsonObject &request);
};

#endif // MINIMAL_SIMPLESERVER_H
