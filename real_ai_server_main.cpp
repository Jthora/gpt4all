#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <csignal>

#include "minimal_simpleserver.h"
#include "gpt4all-chat/src/chat.h"
#include "gpt4all-chat/src/chatllm.h"
#include "gpt4all-chat/src/chatmodel.h"
#include "gpt4all-chat/src/modellist.h"

class RealAIServerApp : public QCoreApplication
{
    Q_OBJECT

public:
    RealAIServerApp(int argc, char *argv[])
        : QCoreApplication(argc, argv)
        , m_server(new MinimalSimpleServer(this))
        , m_chat(nullptr)
    {
        qDebug() << "🚀 Starting REAL AI Server with GPT4All ML Integration...";
        
        // Create real Chat instance (this creates ChatLLM internally)
        m_chat = new Chat(this);
        
        // Connect the server to real AI components
        m_server->setChatLLM(m_chat->chatLLM());
        m_server->setChatModel(m_chat->chatModel());
        
        // Start the HTTP server
        if (m_server->startServer(4891)) {
            qDebug() << "✅ REAL AI Server started on port 4891";
            qDebug() << "🔥 Now serving AUTHENTIC GPT4All AI responses!";
            qDebug() << "📡 Test with: curl -X POST http://localhost:4891/v1/chat/completions";
            qDebug() << "              -H 'Content-Type: application/json'";
            qDebug() << "              -d '{\"model\": \"gpt4all\", \"messages\": [{\"role\": \"user\", \"content\": \"Hello!\"}]}'";
        } else {
            qCritical() << "❌ Failed to start server";
            exit(1);
        }
        
        // Handle Ctrl+C gracefully
        signal(SIGINT, [](int) {
            qDebug() << "\n🛑 Shutting down REAL AI Server...";
            QCoreApplication::quit();
        });
        signal(SIGTERM, [](int) {
            qDebug() << "\n🛑 Shutting down REAL AI Server...";
            QCoreApplication::quit();
        });
    }

private:
    MinimalSimpleServer *m_server;
    Chat *m_chat;
};

int main(int argc, char *argv[])
{
    RealAIServerApp app(argc, argv);
    
    qDebug() << "🎯 REAL AI Server ready! Press Ctrl+C to stop.";
    
    return app.exec();
}

#include "real_ai_server_main.moc"
