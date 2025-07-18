#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

// Include our Minimal SimpleServer implementation
#include "minimal_simpleserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "Starting MinimalSimpleServer test...";
    
    // Create the server
    MinimalSimpleServer server;
    
    // Start the server on port 4891
    if (!server.startServer(4891)) {
        qDebug() << "Failed to start server";
        return 1;
    }
    
    qDebug() << "MinimalSimpleServer started on port 4891";
    qDebug() << "Testing endpoints:";
    qDebug() << "  http://localhost:4891/health";
    qDebug() << "  http://localhost:4891/v1/models";
    
    // Set up a timer to test the server after it starts
    QTimer::singleShot(1000, [&]() {
        QNetworkAccessManager *manager = new QNetworkAccessManager(&app);
        
        // Test health endpoint
        QNetworkRequest request(QUrl("http://localhost:4891/health"));
        QNetworkReply *reply = manager->get(request);
        
        QObject::connect(reply, &QNetworkReply::finished, [reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                qDebug() << "Health check successful:" << reply->readAll();
            } else {
                qDebug() << "Health check failed:" << reply->errorString();
            }
            reply->deleteLater();
        });
    });
    
    // Set up a timer to test models endpoint
    QTimer::singleShot(2000, [&]() {
        QNetworkAccessManager *manager = new QNetworkAccessManager(&app);
        
        // Test models endpoint
        QNetworkRequest request(QUrl("http://localhost:4891/v1/models"));
        QNetworkReply *reply = manager->get(request);
        
        QObject::connect(reply, &QNetworkReply::finished, [reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                qDebug() << "Models endpoint successful:" << reply->readAll();
            } else {
                qDebug() << "Models endpoint failed:" << reply->errorString();
            }
            reply->deleteLater();
        });
    });
    
    // Exit after testing
    QTimer::singleShot(5000, [&]() {
        qDebug() << "Test completed, shutting down...";
        app.quit();
    });
    
    return app.exec();
}
