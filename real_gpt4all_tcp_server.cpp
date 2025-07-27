#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QDebug>
#include <QDateTime>

// Include REAL GPT4All headers
#include "gpt4all-chat/src/chat.h"
#include "gpt4all-chat/src/chatllm.h"
#include "gpt4all-chat/src/modellist.h"
#include "gpt4all-chat/src/mysettings.h"
#include "gpt4all-chat/src/database.h"

class RealGPT4AllAPIServer : public QObject {
    Q_OBJECT

public:
    RealGPT4AllAPIServer(QObject *parent = nullptr) : QObject(parent) {
        initializeGPT4All();
        setupServer();
    }

private:
    void initializeGPT4All() {
        qDebug() << "🚀 Initializing REAL GPT4All components for AI inference...";
        
        // Initialize core GPT4All system
        try {
            // Initialize settings
            m_settings = MySettings::globalInstance();
            if (!m_settings) {
                qWarning() << "❌ Failed to initialize MySettings";
                return;
            }
            
            // Initialize model list
            m_modelList = ModelList::globalInstance();
            if (!m_modelList) {
                qWarning() << "❌ Failed to initialize ModelList";
                return;
            }
            
            // Initialize database
            Database::globalInstance();
            
            // Create chat instance for real AI
            m_chat = new Chat(this);
            if (!m_chat) {
                qWarning() << "❌ Failed to create Chat instance";
                return;
            }
            
            qDebug() << "✅ Core GPT4All components initialized";
            
            // Try to load a model
            tryLoadModel();
            
        } catch (const std::exception &e) {
            qWarning() << "❌ Exception during GPT4All initialization:" << e.what();
        } catch (...) {
            qWarning() << "❌ Unknown exception during GPT4All initialization";
        }
    }
    
    void tryLoadModel() {
        if (!m_chat || !m_modelList) {
            qWarning() << "⚠️  Cannot load model: Chat or ModelList not initialized";
            return;
        }
        
        // Get ChatLLM instance
        ChatLLM *chatLLM = m_chat->chatLLM();
        if (!chatLLM) {
            qWarning() << "⚠️  Cannot load model: ChatLLM not available";
            return;
        }
        
        // Check for installed models
        qDebug() << "🔍 Checking for installed GPT4All models...";
        
        // Simple approach: check common model locations
        QStringList modelPaths = {
            QDir::homePath() + "/.local/share/nomic.ai/GPT4All",
            QDir::homePath() + "/gpt4all_models",
            "/usr/share/gpt4all/models",
            "./models"
        };
        
        QStringList foundModels;
        for (const QString &path : modelPaths) {
            QDir dir(path);
            if (dir.exists()) {
                auto files = dir.entryList(QStringList() << "*.gguf" << "*.bin", QDir::Files);
                for (const QString &file : files) {
                    foundModels << dir.absoluteFilePath(file);
                }
            }
        }
        
        if (foundModels.isEmpty()) {
            qWarning() << "⚠️  No GPT4All models found in standard locations.";
            qWarning() << "     Please download a model from: https://gpt4all.io/models/";
            qWarning() << "     Expected locations:" << modelPaths;
            return;
        }
        
        // Try to load the first found model
        QString modelPath = foundModels.first();
        qDebug() << "🔄 Attempting to load model:" << modelPath;
        
        // Attempt to load model
        bool success = false;
        QMetaObject::invokeMethod(chatLLM, "loadModel",
                                  Qt::DirectConnection,
                                  Q_RETURN_ARG(bool, success),
                                  Q_ARG(QString, modelPath));
        
        if (success && chatLLM->isModelLoaded()) {
            qDebug() << "✅ REAL AI model loaded successfully!";
            qDebug() << "🤖 Model:" << modelPath;
            m_modelLoaded = true;
        } else {
            qWarning() << "❌ Failed to load model:" << modelPath;
        }
    }
    
    void setupServer() {
        m_server = new QTcpServer(this);
        connect(m_server, &QTcpServer::newConnection, this, &RealGPT4AllAPIServer::handleNewConnection);
        
        if (m_server->listen(QHostAddress::Any, 4891)) {
            qDebug() << "✅ REAL GPT4All API Server started on port 4891";
            qDebug() << "🔗 Available endpoints:";
            qDebug() << "   Health: http://localhost:4891/health";
            qDebug() << "   Models: http://localhost:4891/v1/models";
            qDebug() << "   Chat:   http://localhost:4891/v1/chat/completions";
            qDebug() << "";
            qDebug() << "🧪 Test REAL AI with:";
            qDebug() << "   curl -X POST http://localhost:4891/v1/chat/completions \\";
            qDebug() << "        -H \"Content-Type: application/json\" \\";
            qDebug() << "        -d '{\"messages\":[{\"role\":\"user\",\"content\":\"Create a categorized list of all the parts in a car\"}]}'";
        } else {
            qWarning() << "❌ Failed to start server on port 4891";
        }
    }
    
private slots:
    void handleNewConnection() {
        QTcpSocket *client = m_server->nextPendingConnection();
        connect(client, &QTcpSocket::readyRead, [this, client]() {
            QByteArray data = client->readAll();
            processRequest(client, data);
        });
        connect(client, &QTcpSocket::disconnected, client, &QTcpSocket::deleteLater);
    }

private:
    void processRequest(QTcpSocket *client, const QByteArray &data) {
        QString request = QString::fromUtf8(data);
        QStringList lines = request.split("\r\n");
        
        if (lines.isEmpty()) return;
        
        QStringList requestLine = lines[0].split(" ");
        if (requestLine.size() < 2) return;
        
        QString method = requestLine[0];
        QString path = requestLine[1];
        
        qDebug() << "📥 REAL AI Request:" << method << path;
        
        QByteArray response;
        
        if (method == "GET" && path == "/health") {
            response = createHealthResponse();
        } else if (method == "GET" && path == "/v1/models") {
            response = createModelsResponse();
        } else if (method == "POST" && path == "/v1/chat/completions") {
            // Extract JSON body
            int bodyStart = request.indexOf("\r\n\r\n");
            QString jsonBody;
            if (bodyStart != -1) {
                jsonBody = request.mid(bodyStart + 4);
            }
            response = createRealAIResponse(jsonBody);
        } else {
            response = createNotFoundResponse();
        }
        
        client->write(response);
        client->flush();
        client->disconnectFromHost();
    }
    
    QByteArray createHealthResponse() {
        QJsonObject health;
        health["status"] = "ok";
        health["server"] = "REAL-GPT4All-API";
        health["version"] = "1.0";
        health["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        if (m_chat && m_chat->chatLLM()) {
            health["model_loaded"] = m_chat->chatLLM()->isModelLoaded();
            health["ai_inference"] = m_chat->chatLLM()->isModelLoaded() ? "REAL AI available" : "No model loaded";
        } else {
            health["model_loaded"] = false;
            health["ai_inference"] = "ChatLLM not available";
        }
        
        return createHttpResponse(200, QJsonDocument(health).toJson(QJsonDocument::Compact));
    }
    
    QByteArray createModelsResponse() {
        QJsonArray models;
        
        QJsonObject model;
        model["id"] = "gpt4all-real";
        model["object"] = "model";
        model["created"] = 1686935002;
        model["owned_by"] = "nomic-ai";
        models.append(model);
        
        QJsonObject response;
        response["object"] = "list";
        response["data"] = models;
        
        return createHttpResponse(200, QJsonDocument(response).toJson(QJsonDocument::Compact));
    }
    
    QByteArray createRealAIResponse(const QString &jsonBody) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsonBody.toUtf8(), &error);
        
        if (error.error != QJsonParseError::NoError) {
            QJsonObject errorObj;
            errorObj["error"] = "Invalid JSON";
            return createHttpResponse(400, QJsonDocument(errorObj).toJson(QJsonDocument::Compact));
        }
        
        QJsonObject request = doc.object();
        QJsonArray messages = request["messages"].toArray();
        
        QString userPrompt;
        for (const auto &msgVal : messages) {
            QJsonObject msg = msgVal.toObject();
            if (msg["role"].toString() == "user") {
                userPrompt = msg["content"].toString();
            }
        }
        
        if (userPrompt.isEmpty()) {
            QJsonObject errorObj;
            errorObj["error"] = "No user message found";
            return createHttpResponse(400, QJsonDocument(errorObj).toJson(QJsonDocument::Compact));
        }
        
        qDebug() << "🤖 Generating REAL AI response for:" << userPrompt.left(50) << "...";
        
        QString aiResponse = generateRealAIResponse(userPrompt);
        
        // Create OpenAI-compatible response
        QJsonObject responseObj;
        responseObj["id"] = QString("chatcmpl-%1").arg(QDateTime::currentMSecsSinceEpoch());
        responseObj["object"] = "chat.completion";
        responseObj["created"] = QDateTime::currentSecsSinceEpoch();
        responseObj["model"] = "gpt4all-real";
        
        QJsonArray choices;
        QJsonObject choice;
        choice["index"] = 0;
        choice["finish_reason"] = "stop";
        
        QJsonObject message;
        message["role"] = "assistant";
        message["content"] = aiResponse;
        choice["message"] = message;
        
        choices.append(choice);
        responseObj["choices"] = choices;
        
        QJsonObject usage;
        usage["prompt_tokens"] = userPrompt.split(" ").size();
        usage["completion_tokens"] = aiResponse.split(" ").size();
        usage["total_tokens"] = usage["prompt_tokens"].toInt() + usage["completion_tokens"].toInt();
        responseObj["usage"] = usage;
        
        return createHttpResponse(200, QJsonDocument(responseObj).toJson(QJsonDocument::Compact));
    }
    
    QString generateRealAIResponse(const QString &userPrompt) {
        if (!m_chat || !m_chat->chatLLM()) {
            return "Error: GPT4All Chat system not initialized";
        }
        
        ChatLLM *chatLLM = m_chat->chatLLM();
        if (!chatLLM->isModelLoaded()) {
            return "Error: No AI model loaded. Please ensure a GPT4All model is available.";
        }
        
        qDebug() << "🧠 Calling REAL GPT4All AI inference...";
        
        try {
            // Attempt real AI inference using GPT4All ChatLLM
            QString response;
            
            // Method 1: Try direct prompt method
            bool success = QMetaObject::invokeMethod(chatLLM, "prompt",
                                                    Qt::DirectConnection,
                                                    Q_RETURN_ARG(QString, response),
                                                    Q_ARG(QString, userPrompt));
            
            if (success && !response.isEmpty()) {
                qDebug() << "✅ REAL AI response generated via prompt method";
                return response;
            }
            
            // Method 2: Try setting prompt and getting response
            chatLLM->setProperty("prompt", userPrompt);
            QMetaObject::invokeMethod(chatLLM, "regenerateResponse", Qt::DirectConnection);
            
            // Get the response
            QVariant responseVar = chatLLM->property("response");
            if (responseVar.isValid() && !responseVar.toString().isEmpty()) {
                qDebug() << "✅ REAL AI response generated via property method";
                return responseVar.toString();
            }
            
            // If we get here, the AI model didn't generate a response
            qWarning() << "⚠️  AI model failed to generate response";
            return QString("I'm a real GPT4All AI model, but I encountered an issue generating a response to: \"%1\". This indicates the model may need to be properly loaded or there may be a communication issue with the inference engine.").arg(userPrompt);
            
        } catch (const std::exception &e) {
            qWarning() << "❌ Exception during AI inference:" << e.what();
            return QString("Error: Exception occurred during AI inference: %1").arg(e.what());
        } catch (...) {
            qWarning() << "❌ Unknown exception during AI inference";
            return "Error: Unknown exception occurred during AI inference";
        }
    }
    
    QByteArray createNotFoundResponse() {
        QJsonObject error;
        error["error"] = "Not Found";
        error["message"] = "The requested endpoint was not found";
        return createHttpResponse(404, QJsonDocument(error).toJson(QJsonDocument::Compact));
    }
    
    QByteArray createHttpResponse(int statusCode, const QByteArray &body) {
        QString statusText;
        switch (statusCode) {
            case 200: statusText = "OK"; break;
            case 400: statusText = "Bad Request"; break;
            case 404: statusText = "Not Found"; break;
            default: statusText = "Unknown"; break;
        }
        
        QByteArray response;
        response += QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(statusText).toUtf8();
        response += "Content-Type: application/json\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
        response += "Access-Control-Allow-Headers: Content-Type\r\n";
        response += QString("Content-Length: %1\r\n").arg(body.length()).toUtf8();
        response += "\r\n";
        response += body;
        
        return response;
    }

private:
    QTcpServer *m_server = nullptr;
    Chat *m_chat = nullptr;
    MySettings *m_settings = nullptr;
    ModelList *m_modelList = nullptr;
    bool m_modelLoaded = false;
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    qDebug() << "🚀 Starting REAL GPT4All API Server...";
    qDebug() << "   This server connects to actual GPT4All AI inference engine";
    
    RealGPT4AllAPIServer server;
    
    return app.exec();
}

#include "real_gpt4all_tcp_server.moc"
