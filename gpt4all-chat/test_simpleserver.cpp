#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QTcpServer>
#include <QHostAddress>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "Testing basic QTcpServer...";
    
    QTcpServer server;
    bool success = server.listen(QHostAddress::LocalHost, 4891);
    
    if (success) {
        qDebug() << "QTcpServer started successfully on port" << server.serverPort();
        
        // Run for 10 seconds then exit
        QTimer::singleShot(10000, &app, &QCoreApplication::quit);
        return app.exec();
    } else {
        qDebug() << "Failed to start QTcpServer:" << server.errorString();
        return 1;
    }
}
