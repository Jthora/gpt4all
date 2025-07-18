#ifndef QTCPHTTPSERVER_H
#define QTCPHTTPSERVER_H

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
#include <functional>

class ChatLLM;

class QTcpHttpServer : public QObject
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

    using RequestHandler = std::function<HttpResponse(const HttpRequest&)>;

    explicit QTcpHttpServer(QObject *parent = nullptr);
    ~QTcpHttpServer();

    bool listen(const QHostAddress &address = QHostAddress::LocalHost, quint16 port = 4891);
    void close();
    bool isListening() const;
    quint16 serverPort() const;

    void addRoute(HttpMethod method, const QString &path, RequestHandler handler);
    void setChatLLM(ChatLLM *chatLLM) { m_chatLLM = chatLLM; }

signals:
    void requestReceived(const QString &method, const QString &path);
    void errorOccurred(const QString &error);

private slots:
    void onNewConnection();
    void onClientDataReady();
    void onClientDisconnected();

private:
    QTcpServer *m_server;
    ChatLLM *m_chatLLM;
    QMap<QString, RequestHandler> m_routes;
    QMutex m_routesMutex;

    HttpRequest parseHttpRequest(const QByteArray &data);
    QByteArray createHttpResponse(const HttpResponse &response);
    QString methodToString(HttpMethod method) const;
    HttpMethod stringToMethod(const QString &method) const;
    
    // Built-in handlers for OpenAI-compatible API
    HttpResponse handleModels(const HttpRequest &request);
    HttpResponse handleChatCompletions(const HttpRequest &request);
    HttpResponse handleHealth(const HttpRequest &request);
    HttpResponse handleCors(const HttpRequest &request);
    
    void setupDefaultRoutes();
};

#endif // QTCPHTTPSERVER_H
