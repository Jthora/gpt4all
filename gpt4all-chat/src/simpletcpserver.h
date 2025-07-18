#ifndef SIMPLETCPSERVER_H
#define SIMPLETCPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QHostAddress>
#include <QDebug>
#include <QDateTime>
#include <functional>

class ChatLLM;

class SimpleTcpServer : public QTcpServer
{
    Q_OBJECT

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

    explicit SimpleTcpServer(QObject *parent = nullptr);
    ~SimpleTcpServer();

    void setChatLLM(ChatLLM *chatLLM) { m_chatLLM = chatLLM; }
    bool startServer(quint16 port = 4891);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onClientDataReady();
    void onClientDisconnected();

private:
    ChatLLM *m_chatLLM;
    
    HttpRequest parseHttpRequest(const QByteArray &data);
    QByteArray createHttpResponse(const HttpResponse &response);
    HttpResponse handleRequest(const HttpRequest &request);
    
    // API handlers
    HttpResponse handleHealth(const HttpRequest &request);
    HttpResponse handleModels(const HttpRequest &request);
    HttpResponse handleChatCompletions(const HttpRequest &request);
    HttpResponse handleCors(const HttpRequest &request);
};

#endif // SIMPLETCPSERVER_H
