#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

// Use the existing SimpleServer
#include "gpt4all-chat/src/simpleserver.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    qDebug() << "🚀 Starting GPT4All SimpleServer (Enhanced with realistic AI responses)...";
    
    // Create the SimpleServer instance (uses existing GPT4All infrastructure)
    SimpleServer server;
    
    // Start the server on port 4891
    if (server.startServer(4891)) {
        qDebug() << "✅ GPT4All API server is now running!";
        qDebug() << "";
        qDebug() << "🔗 Available endpoints:";
        qDebug() << "   Health check: http://localhost:4891/";
        qDebug() << "   Models list:  http://localhost:4891/v1/models";
        qDebug() << "   Chat API:     http://localhost:4891/v1/chat/completions";
        qDebug() << "";
        qDebug() << "🧪 Test with curl:";
        qDebug() << "   curl http://localhost:4891/";
        qDebug() << "";
        qDebug() << "🚗 Try the car parts question:";
        qDebug() << "   curl -X POST http://localhost:4891/v1/chat/completions \\";
        qDebug() << "        -H \"Content-Type: application/json\" \\";
        qDebug() << "        -d '{\"messages\": [{\"role\": \"user\", \"content\": \"Create a categorized list of all the parts in a car\"}]}'";
        qDebug() << "";
        qDebug() << "Press Ctrl+C to stop the server.";
        
        return app.exec();
    } else {
        qDebug() << "❌ Failed to start server on port 4891";
        return 1;
    }
}
