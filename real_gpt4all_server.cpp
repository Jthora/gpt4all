#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <memory>

// Include GPT4All headers
#include "gpt4all-chat/src/chat.h"
#include "gpt4all-chat/src/chatllm.h"
#include "gpt4all-chat/src/modellist.h"
#include "gpt4all-chat/src/database.h"
#include "gpt4all-chat/src/mysettings.h"

class GPT4AllAPIServer : public QObject {
    Q_OBJECT

public:
    GPT4AllAPIServer(QObject *parent = nullptr) : QObject(parent) {
        initializeGPT4All();
        setupServer();
    }

private:
    void initializeGPT4All() {
        qDebug() << "Initializing GPT4All components for REAL AI inference...";
        
        // Initialize the settings first
        m_settings = MySettings::globalInstance();
        
        // Initialize model list to discover available models
        m_modelList = ModelList::globalInstance();
        
        // Create a chat instance
        m_chat = new Chat(this);
        
        // Try to load a default model if available
        tryLoadDefaultModel();
        
        qDebug() << "GPT4All components initialized";
    }
    
    void tryLoadDefaultModel() {
        if (!m_chat || !m_modelList) {
            qDebug() << "⚠️  Cannot load model: Chat or ModelList not initialized";
            return;
        }
        
        // Get the ChatLLM instance
        ChatLLM *chatLLM = m_chat->chatLLM();
        if (!chatLLM) {
            qDebug() << "⚠️  Cannot load model: ChatLLM not available";
            return;
        }
        
        // First, try to load using ModelList's installed models
        qDebug() << "🔍 Checking ModelList for installed models...";
        auto installedModels = m_modelList->installedModels();
        
        if (!installedModels.isEmpty()) {
            auto firstModel = installedModels.first();
            qDebug() << "🔄 Attempting to load model from ModelList:" << firstModel.filename();
            
            // Use Chat's model loading mechanism
            bool loadSuccess = QMetaObject::invokeMethod(chatLLM, "loadModel",
                                                        Qt::DirectConnection,
                                                        Q_RETURN_ARG(bool, loadSuccess),
                                                        Q_ARG(ModelInfo, firstModel));
            
            if (loadSuccess && chatLLM->isModelLoaded()) {
                qDebug() << "✅ Model loaded successfully from ModelList:" << firstModel.filename();
                qDebug() << "🤖 REAL AI inference is now available!";
                return;
            }
            
            qDebug() << "❌ Failed to load model from ModelList:" << firstModel.filename();
        } else {
            qDebug() << "⚠️  No models found in ModelList";
        }
        
        // Fallback: Try to find models manually in common locations
        qDebug() << "🔍 Searching for models in common locations...";
        QStringList searchPaths = {
            QDir::homePath() + "/.local/share/nomic.ai/GPT4All",
            QDir::homePath() + "/gpt4all_models",
            "/usr/share/gpt4all/models",
            "./models",
            "/home/jono/Applications/GPT4All"  // Found model here earlier
        };
        
        QStringList foundModels;
        for (const QString &searchPath : searchPaths) {
            QDir dir(searchPath);
            if (dir.exists()) {
                auto files = dir.entryList(QStringList() << "*.gguf" << "*.bin", QDir::Files);
                for (const QString &file : files) {
                    foundModels << dir.absoluteFilePath(file);
                    qDebug() << "   Found model:" << dir.absoluteFilePath(file);
                }
            }
        }
        
        if (!foundModels.isEmpty()) {
            QString modelPath = foundModels.first();
            qDebug() << "🔄 Attempting to load model manually:" << modelPath;
            
            // Create ModelInfo for manual loading
            ModelInfo modelInfo;
            modelInfo.setFilename(QFileInfo(modelPath).fileName());
            modelInfo.setFilepath(modelPath);
            
            bool loadSuccess = QMetaObject::invokeMethod(chatLLM, "loadModel",
                                                        Qt::DirectConnection,
                                                        Q_RETURN_ARG(bool, loadSuccess),
                                                        Q_ARG(ModelInfo, modelInfo));
            
            if (loadSuccess && chatLLM->isModelLoaded()) {
                qDebug() << "✅ Model loaded successfully manually:" << modelPath;
                qDebug() << "🤖 REAL AI inference is now available!";
                return;
            }
            
            qDebug() << "❌ Failed to load model manually:" << modelPath;
        }
        
        // If we get here, no models could be loaded
        qDebug() << "❌ No GPT4All models could be loaded.";
        qDebug() << "     Please download a model from: https://gpt4all.io/models/";
        qDebug() << "     Expected locations:" << searchPaths;
    }
    
    void setupServer() {
        m_tcpServer = new QTcpServer(this);
        
        connect(m_tcpServer, &QTcpServer::newConnection, this, &GPT4AllAPIServer::handleNewConnection);
        
        // Listen on port 4891
        if (m_tcpServer->listen(QHostAddress::Any, 4891)) {
            qDebug() << "GPT4All API Server listening on port 4891";
            qDebug() << "Try: curl http://localhost:4891/health";
        } else {
            qDebug() << "Failed to start server on port 4891";
        }
    }
    
    void handleNewConnection() {
        QTcpSocket *socket = m_tcpServer->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, [this, socket]() {
            QByteArray data = socket->readAll();
            QString request = QString::fromUtf8(data);
            
            QString response;
            if (request.contains("POST /v1/chat/completions")) {
                response = handleChatCompletionTcp(request);
            } else if (request.contains("GET /health")) {
                response = handleHealthTcp();
            } else if (request.contains("GET /v1/models")) {
                response = handleModelsTcp();
            } else {
                response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            }
            
            socket->write(response.toUtf8());
            socket->disconnectFromHost();
        });
    }
    
    QString handleChatCompletionTcp(const QString &request) {
        // Extract JSON body from HTTP request
        int bodyStart = request.indexOf("\r\n\r\n") + 4;
        QString jsonBody = request.mid(bodyStart);
        
        QJsonParseError error;
        auto document = QJsonDocument::fromJson(jsonBody.toUtf8(), &error);
        
        if (error.error != QJsonParseError::NoError) {
            return "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Invalid JSON\"}";
        }
        
        auto requestObj = document.object();
        auto messages = requestObj["messages"].toArray();
        
        if (messages.isEmpty()) {
            return "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\n\r\n{\"error\":\"No messages provided\"}";
        }
        
        // Get the last user message
        QString prompt;
        for (const auto &msg : messages) {
            auto msgObj = msg.toObject();
            if (msgObj["role"].toString() == "user") {
                prompt = msgObj["content"].toString();
            }
        }
        
        if (prompt.isEmpty()) {
            return "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\n\r\n{\"error\":\"No user message found\"}";
        }
        
        qDebug() << "Processing prompt:" << prompt;
        
        // Use GPT4All to generate response - KEEPING ALL REAL AI LOGIC
        QString aiResponse = generateResponse(prompt);
        
        // Format as OpenAI-compatible response
        QJsonObject chatResponse;
        chatResponse["id"] = "chatcmpl-" + QString::number(QDateTime::currentSecsSinceEpoch());
        chatResponse["object"] = "chat.completion";
        chatResponse["created"] = QDateTime::currentSecsSinceEpoch();
        chatResponse["model"] = "gpt4all";
        
        QJsonObject choice;
        QJsonObject message;
        message["role"] = "assistant";
        message["content"] = aiResponse;
        choice["index"] = 0;
        choice["message"] = message;
        choice["finish_reason"] = "stop";
        
        QJsonArray choices;
        choices.append(choice);
        chatResponse["choices"] = choices;
        
        QJsonObject usage;
        usage["prompt_tokens"] = prompt.length() / 4;
        usage["completion_tokens"] = aiResponse.length() / 4;
        usage["total_tokens"] = usage["prompt_tokens"].toInt() + usage["completion_tokens"].toInt();
        chatResponse["usage"] = usage;
        
        QJsonDocument responseDoc(chatResponse);
        QByteArray responseData = responseDoc.toJson();
        
        return QString("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %1\r\n\r\n%2")
               .arg(responseData.size())
               .arg(QString::fromUtf8(responseData));
    }
    
    QString handleHealthTcp() {
        QJsonObject response;
        response["status"] = "healthy";
        response["chat_ready"] = m_chat ? true : false;
        
        if (m_chat && m_chat->chatLLM()) {
            response["model_loaded"] = m_chat->chatLLM()->isModelLoaded();
            if (m_chat->chatLLM()->isModelLoaded()) {
                response["ai_inference"] = "REAL AI available";
            } else {
                response["ai_inference"] = "No model loaded";
            }
        } else {
            response["model_loaded"] = false;
            response["ai_inference"] = "ChatLLM not available";
        }
        
        QJsonDocument responseDoc(response);
        QByteArray responseData = responseDoc.toJson();
        
        return QString("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %1\r\n\r\n%2")
               .arg(responseData.size())
               .arg(QString::fromUtf8(responseData));
    }
    
    QString handleModelsTcp() {
        QJsonArray models;
        
        QJsonObject model;
        model["id"] = "gpt4all";
        model["object"] = "model";
        model["created"] = 1686935002;
        model["owned_by"] = "nomic-ai";
        models.append(model);
        
        QJsonObject response;
        response["object"] = "list";
        response["data"] = models;
        
        QJsonDocument responseDoc(response);
        QByteArray responseData = responseDoc.toJson();
        
        return QString("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %1\r\n\r\n%2")
               .arg(responseData.size())
               .arg(QString::fromUtf8(responseData));
    }
    
    QString generateResponse(const QString &prompt) {
        if (!m_chat) {
            return "Error: Chat system not initialized";
        }
        
        // Get the ChatLLM instance from the chat
        ChatLLM *chatLLM = m_chat->chatLLM();
        if (!chatLLM) {
            return "Error: ChatLLM not available";
        }
        
        // Check if a model is loaded
        if (!chatLLM->isModelLoaded()) {
            return "Error: No AI model loaded. Please load a GPT4All model first.";
        }
        
        qDebug() << "🤖 Generating REAL AI response for:" << prompt.left(50) << "...";
        
        try {
            // Method 1: Use Chat's newPromptResponsePair method (the proper GPT4All way)
            QMetaObject::invokeMethod(m_chat, "newPromptResponsePair",
                                      Qt::DirectConnection,
                                      Q_ARG(QString, prompt));
            
            // Wait for the response to be generated
            // In a real implementation, this should be async, but for demo we'll wait briefly
            QCoreApplication::processEvents();
            
            // Get the response from the chat model
            QString response;
            QMetaObject::invokeMethod(m_chat->chatModel(), "responseText",
                                      Qt::DirectConnection,
                                      Q_RETURN_ARG(QString, response));
            
            if (!response.isEmpty()) {
                qDebug() << "✅ REAL AI response generated via Chat.newPromptResponsePair";
                return response;
            }
            
            // Method 2: Direct ChatLLM prompt with collections
            QStringList enabledCollections; // Empty for basic prompting
            QMetaObject::invokeMethod(chatLLM, "prompt",
                                      Qt::DirectConnection,
                                      Q_ARG(QStringList, enabledCollections));
            
            // Process events to allow AI generation
            QCoreApplication::processEvents();
            
            // Try to get response from ChatLLM
            QVariant responseVar = chatLLM->property("responseText");
            if (responseVar.isValid() && !responseVar.toString().isEmpty()) {
                qDebug() << "✅ REAL AI response generated via ChatLLM.prompt";
                return responseVar.toString();
            }
            
            // Method 3: Check if we can get the latest response from chat model
            if (m_chat->chatModel() && m_chat->chatModel()->count() > 0) {
                QString lastResponse = m_chat->chatModel()->responseText();
                if (!lastResponse.isEmpty()) {
                    qDebug() << "✅ REAL AI response retrieved from chat model";
                    return lastResponse;
                }
            }
            
            // If all methods fail, but model is loaded, report the issue
            qWarning() << "⚠️  AI model loaded but failed to generate response";
            return QString("AI model is loaded and ready, but failed to generate a response for: \"%1\". This may indicate the model needs more initialization time or the prompt format needs adjustment.").arg(prompt);
            
        } catch (const std::exception &e) {
            qWarning() << "❌ Exception during AI inference:" << e.what();
            return QString("Error: Exception occurred during AI inference: %1").arg(e.what());
        } catch (...) {
            qWarning() << "❌ Unknown exception during AI inference";
            return "Error: Unknown exception occurred during AI inference";
        }
    }

private:
    QTcpServer *m_tcpServer = nullptr;
    Chat *m_chat = nullptr;
    MySettings *m_settings = nullptr;
    ModelList *m_modelList = nullptr;
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    qDebug() << "Starting GPT4All API Server...";
    
    GPT4AllAPIServer server;
    
    return app.exec();
}

#include "real_gpt4all_server.moc"
