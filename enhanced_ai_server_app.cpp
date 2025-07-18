// =============================================================================
// ENHANCED AI SERVER TEST APPLICATION
// Complete test app showing real AI integration with GPT4All
// =============================================================================

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QDir>

#include "enhanced_ai_server.h"
#include "chatllm.h"
#include "modellist.h"
#include "database.h"
#include "chat.h"
#include "mysettings.h"

class EnhancedAIServerApp : public QObject
{
    Q_OBJECT

public:
    EnhancedAIServerApp(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void start()
    {
        qDebug() << "🚀 Starting Enhanced AI Server with REAL GPT4All integration...";
        
        // Initialize GPT4All components
        initializeGPT4AllComponents();
        
        // Create and configure server
        m_server = new EnhancedAIServer(this);
        
        // Connect AI components to server
        m_server->setChatLLM(m_chatLLM);
        m_server->setModelList(m_modelList);
        m_server->setDatabase(m_database);
        m_server->setChat(m_chat);
        
        // Connect server signals
        connect(m_server, &EnhancedAIServer::requestReceived, this, &EnhancedAIServerApp::onRequestReceived);
        connect(m_server, &EnhancedAIServer::errorOccurred, this, &EnhancedAIServerApp::onError);
        connect(m_server, &EnhancedAIServer::aiResponseGenerated, this, &EnhancedAIServerApp::onAIResponseGenerated);
        
        // Start server
        bool started = m_server->listen(QHostAddress::LocalHost, 4891);
        if (started) {
            qDebug() << "✅ Enhanced AI Server started successfully!";
            qDebug() << "";
            qDebug() << "🔥 REAL AI API ENDPOINTS AVAILABLE:";
            qDebug() << "   Health: http://localhost:4891/";
            qDebug() << "   Models: http://localhost:4891/v1/models";
            qDebug() << "   Chat:   http://localhost:4891/v1/chat/completions";
            qDebug() << "";
            qDebug() << "💡 TEST WITH CURL:";
            qDebug() << "curl -X POST http://localhost:4891/v1/chat/completions \\";
            qDebug() << "  -H \"Content-Type: application/json\" \\";
            qDebug() << "  -d '{\"model\":\"gpt4all-local\",\"messages\":[{\"role\":\"user\",\"content\":\"Hello AI!\"}]}'";
            qDebug() << "";
            qDebug() << "🎯 This server now has REAL AI integration!";
            
        } else {
            qCritical() << "❌ Failed to start Enhanced AI Server";
            QCoreApplication::exit(1);
        }
    }

private slots:
    void onRequestReceived(const QString &method, const QString &path)
    {
        qDebug() << "📨" << method << path;
    }
    
    void onError(const QString &error)
    {
        qWarning() << "❌ Server error:" << error;
    }
    
    void onAIResponseGenerated(const QString &sessionId, const QString &response)
    {
        qDebug() << "🤖 AI Response generated for session" << sessionId << ":" << response.left(100);
    }

private:
    void initializeGPT4AllComponents()
    {
        qDebug() << "🔧 Initializing GPT4All components...";
        
        // Initialize settings
        MySettings::globalInstance();
        
        // Create Chat instance (this creates ChatLLM automatically)
        m_chat = new Chat(this);
        m_chatLLM = m_chat->chatModel() ? qobject_cast<ChatLLM*>(m_chat->chatModel()->parent()) : nullptr;
        
        if (!m_chatLLM) {
            // Fallback: create ChatLLM directly
            m_chatLLM = new ChatLLM(m_chat, true /* isServer */);
        }
        
        // Initialize ModelList
        m_modelList = ModelList::globalInstance();
        
        // Initialize Database
        m_database = Database::globalInstance();
        
        qDebug() << "✅ GPT4All components initialized";
        qDebug() << "   ChatLLM:" << (m_chatLLM ? "✅" : "❌");
        qDebug() << "   ModelList:" << (m_modelList ? "✅" : "❌");
        qDebug() << "   Database:" << (m_database ? "✅" : "❌");
        qDebug() << "   Chat:" << (m_chat ? "✅" : "❌");
    }

private:
    EnhancedAIServer *m_server = nullptr;
    ChatLLM *m_chatLLM = nullptr;
    ModelList *m_modelList = nullptr;
    Database *m_database = nullptr;
    Chat *m_chat = nullptr;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "🎯 Enhanced AI Server - REAL GPT4All Integration";
    qDebug() << "================================================";
    
    EnhancedAIServerApp serverApp;
    
    // Start the server after event loop starts
    QTimer::singleShot(0, &serverApp, &EnhancedAIServerApp::start);
    
    return app.exec();
}

#include "enhanced_ai_server_app.moc"
