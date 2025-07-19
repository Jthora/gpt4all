#ifndef SIMPLESERVER_H
#define SIMPLESERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QHostAddress>
#include <QDebug>
#include <QDateTime>

class ChatLLM;

// Simple HTTP server without Q_OBJECT to avoid MOC issues
class SimpleServer
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

    explicit SimpleServer();
    ~SimpleServer();

    void setChatLLM(ChatLLM *chatLLM);
    bool startServer(quint16 port = 4891);
    void stopServer();
    bool isListening() const;

private:
    QTcpServer *m_server;
    ChatLLM *m_chatLLM;
    
    void handleNewConnection();
    void handleClientData(QTcpSocket *client);
    
    HttpRequest parseHttpRequest(const QByteArray &data);
    QByteArray createHttpResponse(const HttpResponse &response);
    HttpResponse handleRequest(const HttpRequest &request);
    
    // AI Integration
    QString generateAIResponse(const QString &userPrompt);
    
    // API handlers
    HttpResponse handleHealth(const HttpRequest &request);
    HttpResponse handleModels(const HttpRequest &request);
    HttpResponse handleChatCompletions(const HttpRequest &request);
    HttpResponse handleCors(const HttpRequest &request);
};

#endif // SIMPLESERVER_H
