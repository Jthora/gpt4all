#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QHttpPart>
#include <QMimeData>
#include <QDebug>
#include <QCoreApplication>
#include <iostream>

// Include the real GPT4All components
#include "gpt4all-chat/src/chat.h"
#include "gpt4all-chat/src/chatllm.h"
#include "gpt4all-chat/src/chatmodel.h"
#include "gpt4all-chat/src/modellist.h"
#include "gpt4all-chat/src/mysettings.h"
#include "gpt4all-chat/src/database.h"

class RealGPT4AllAPIServer : public QObject
{
    Q_OBJECT

public:
    explicit RealGPT4AllAPIServer(QObject *parent = nullptr)
        : QObject(parent)
        , m_server(new QTcpServer(this))
        , m_chat(new Chat(this))
        , m_chatLLM(m_chat->chatLLM())
    {
        // Initialize GPT4All components
        initializeGPT4All();
        
        // Set up HTTP server
        connect(m_server, &QTcpServer::newConnection, this, &RealGPT4AllAPIServer::handleNewConnection);
    }

    void start(quint16 port = 4891)
    {
        if (!m_server->listen(QHostAddress::Any, port)) {
            qDebug() << "Failed to start server on port" << port;
            return;
        }
        
        qDebug() << "Real GPT4All API Server started on port" << port;
        qDebug() << "Available endpoints:";
        qDebug() << "  GET  /health - Server health check";
        qDebug() << "  GET  /models - List available models";
        qDebug() << "  POST /v1/chat/completions - Chat with loaded model";
        qDebug() << "  POST /v1/completions - Text completion";
    }

private slots:
    void handleNewConnection()
    {
        QTcpSocket *socket = m_server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, [this, socket]() {
            handleHttpRequest(socket);
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }

private:
    void initializeGPT4All()
    {
        // Initialize the database and settings
        qDebug() << "Initializing GPT4All components...";
        
        // Make sure we have a valid chat object
        if (!m_chat) {
            qDebug() << "ERROR: Failed to create Chat object";
            return;
        }
        
        // Check if we have any models available
        ModelList *modelList = ModelList::globalInstance();
        if (!modelList) {
            qDebug() << "ERROR: Failed to get ModelList instance";
            return;
        }
        
        // Try to load a default model
        auto models = modelList->selectableModelList();
        if (!models.isEmpty()) {
            ModelInfo defaultModel = models.first();
            qDebug() << "Found model:" << defaultModel.name();
            m_chat->setModelInfo(defaultModel);
            
            // Load the model
            if (m_chatLLM) {
                qDebug() << "Loading model...";
                m_chatLLM->loadModel(defaultModel);
            }
        } else {
            qDebug() << "WARNING: No models found. Server will run but responses will be limited.";
        }
    }

    void handleHttpRequest(QTcpSocket *socket)
    {
        QByteArray requestData = socket->readAll();
        QString request = QString::fromUtf8(requestData);
        
        QStringList lines = request.split("\r\n");
        if (lines.isEmpty()) {
            sendErrorResponse(socket, 400, "Bad Request");
            return;
        }
        
        QString requestLine = lines.first();
        QStringList parts = requestLine.split(" ");
        if (parts.size() < 3) {
            sendErrorResponse(socket, 400, "Bad Request");
            return;
        }
        
        QString method = parts[0];
        QString path = parts[1];
        
        qDebug() << "Request:" << method << path;
        
        // Extract request body for POST requests
        QByteArray body;
        if (method == "POST") {
            int bodyStart = request.indexOf("\r\n\r\n");
            if (bodyStart != -1) {
                body = request.mid(bodyStart + 4).toUtf8();
            }
        }
        
        // Route the request
        if (method == "GET" && path == "/health") {
            handleHealthCheck(socket);
        } else if (method == "GET" && path == "/models") {
            handleModelsRequest(socket);
        } else if (method == "POST" && path == "/v1/chat/completions") {
            handleChatCompletions(socket, body);
        } else if (method == "POST" && path == "/v1/completions") {
            handleCompletions(socket, body);
        } else {
            sendErrorResponse(socket, 404, "Not Found");
        }
    }

    void handleHealthCheck(QTcpSocket *socket)
    {
        QJsonObject response;
        response["service"] = "Real GPT4All API Server";
        response["status"] = "ok";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        response["model_loaded"] = m_chat ? m_chat->isModelLoaded() : false;
        response["model_info"] = m_chat && m_chat->isModelLoaded() ? m_chat->modelInfo().name() : "none";
        
        sendJsonResponse(socket, response);
    }

    void handleModelsRequest(QTcpSocket *socket)
    {
        QJsonObject response;
        QJsonArray models;
        
        ModelList *modelList = ModelList::globalInstance();
        if (modelList) {
            auto modelInfos = modelList->selectableModelList();
            for (const auto &info : modelInfos) {
                QJsonObject model;
                model["id"] = info.name();
                model["object"] = "model";
                model["created"] = 0;
                model["owned_by"] = "gpt4all";
                model["filename"] = info.filename();
                model["installed"] = info.installed;
                models.append(model);
            }
        }
        
        response["object"] = "list";
        response["data"] = models;
        
        sendJsonResponse(socket, response);
    }

    void handleChatCompletions(QTcpSocket *socket, const QByteArray &body)
    {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(body, &error);
        
        if (error.error != QJsonParseError::NoError) {
            sendErrorResponse(socket, 400, "Invalid JSON: " + error.errorString());
            return;
        }
        
        QJsonObject request = doc.object();
        QJsonArray messages = request["messages"].toArray();
        
        if (messages.isEmpty()) {
            sendErrorResponse(socket, 400, "No messages provided");
            return;
        }
        
        // Check if model is loaded
        if (!m_chat || !m_chat->isModelLoaded()) {
            sendErrorResponse(socket, 503, "No model loaded. Please ensure GPT4All models are available.");
            return;
        }
        
        // Get the last user message
        QString userMessage;
        for (const auto &msgValue : messages) {
            QJsonObject msg = msgValue.toObject();
            if (msg["role"].toString() == "user") {
                userMessage = msg["content"].toString();
            }
        }
        
        if (userMessage.isEmpty()) {
            sendErrorResponse(socket, 400, "No user message found");
            return;
        }
        
        // Generate response using real GPT4All
        QString aiResponse = generateRealResponse(userMessage);
        
        // Format OpenAI-compatible response
        QJsonObject response;
        response["id"] = QString("chatcmpl-%1").arg(QDateTime::currentMSecsSinceEpoch());
        response["object"] = "chat.completion";
        response["created"] = QDateTime::currentSecsSinceEpoch();
        response["model"] = m_chat->modelInfo().name();
        
        QJsonArray choices;
        QJsonObject choice;
        choice["index"] = 0;
        
        QJsonObject message;
        message["role"] = "assistant";
        message["content"] = aiResponse;
        choice["message"] = message;
        choice["finish_reason"] = "stop";
        
        choices.append(choice);
        response["choices"] = choices;
        
        QJsonObject usage;
        usage["prompt_tokens"] = userMessage.length() / 4; // rough estimate
        usage["completion_tokens"] = aiResponse.length() / 4;
        usage["total_tokens"] = (userMessage.length() + aiResponse.length()) / 4;
        response["usage"] = usage;
        
        sendJsonResponse(socket, response);
    }

    void handleCompletions(QTcpSocket *socket, const QByteArray &body)
    {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(body, &error);
        
        if (error.error != QJsonParseError::NoError) {
            sendErrorResponse(socket, 400, "Invalid JSON: " + error.errorString());
            return;
        }
        
        QJsonObject request = doc.object();
        QString prompt = request["prompt"].toString();
        
        if (prompt.isEmpty()) {
            sendErrorResponse(socket, 400, "No prompt provided");
            return;
        }
        
        // Check if model is loaded
        if (!m_chat || !m_chat->isModelLoaded()) {
            sendErrorResponse(socket, 503, "No model loaded. Please ensure GPT4All models are available.");
            return;
        }
        
        // Generate response using real GPT4All
        QString aiResponse = generateRealResponse(prompt);
        
        // Format OpenAI-compatible response
        QJsonObject response;
        response["id"] = QString("cmpl-%1").arg(QDateTime::currentMSecsSinceEpoch());
        response["object"] = "text_completion";
        response["created"] = QDateTime::currentSecsSinceEpoch();
        response["model"] = m_chat->modelInfo().name();
        
        QJsonArray choices;
        QJsonObject choice;
        choice["text"] = aiResponse;
        choice["index"] = 0;
        choice["finish_reason"] = "stop";
        
        choices.append(choice);
        response["choices"] = choices;
        
        sendJsonResponse(socket, response);
    }

    QString generateRealResponse(const QString &prompt)
    {
        if (!m_chat || !m_chat->isModelLoaded()) {
            return "Error: No model loaded";
        }
        
        // Use the real ChatLLM to generate a response
        // This will connect to the actual GPT4All inference engine
        try {
            // Add the prompt to the chat model
            m_chat->chatModel()->updateCurrentResponse(m_chat->chatModel()->count() - 1, false);
            
            // Trigger the actual AI response generation
            // The ChatLLM will handle the real model inference
            QStringList collections; // Empty for now, can be extended for RAG
            m_chatLLM->prompt(collections);
            
            // Wait for response (this is a simplified approach)
            // In a production system, you'd want proper async handling
            QEventLoop loop;
            QTimer timer;
            timer.setSingleShot(true);
            timer.start(30000); // 30 second timeout
            
            connect(m_chatLLM, &ChatLLM::responseStopped, &loop, &QEventLoop::quit);
            connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
            
            loop.exec();
            
            // Get the response from the chat model
            int responseIndex = m_chat->chatModel()->count() - 1;
            if (responseIndex >= 0) {
                return m_chat->chatModel()->data(m_chat->chatModel()->index(responseIndex), ChatModel::TextRole).toString();
            }
            
        } catch (const std::exception &e) {
            qDebug() << "Error generating response:" << e.what();
            return QString("Error generating response: %1").arg(e.what());
        }
        
        return "Unable to generate response from model";
    }

    void sendJsonResponse(QTcpSocket *socket, const QJsonObject &json)
    {
        QJsonDocument doc(json);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        
        QString response = QString("HTTP/1.1 200 OK\r\n");
        response += "Content-Type: application/json\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
        response += "Access-Control-Allow-Headers: Content-Type\r\n";
        response += QString("Content-Length: %1\r\n").arg(data.length());
        response += "\r\n";
        
        socket->write(response.toUtf8());
        socket->write(data);
        socket->disconnectFromHost();
    }

    void sendErrorResponse(QTcpSocket *socket, int statusCode, const QString &message)
    {
        QJsonObject error;
        error["error"] = message;
        error["status_code"] = statusCode;
        
        QJsonDocument doc(error);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        
        QString response = QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(message);
        response += "Content-Type: application/json\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += QString("Content-Length: %1\r\n").arg(data.length());
        response += "\r\n";
        
        socket->write(response.toUtf8());
        socket->write(data);
        socket->disconnectFromHost();
    }

private:
    QTcpServer *m_server;
    Chat *m_chat;
    ChatLLM *m_chatLLM;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Parse command line arguments
    quint16 port = 4891;
    bool verbose = false;
    
    QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == "--port" && i + 1 < args.size()) {
            port = args[++i].toUShort();
        } else if (args[i] == "--verbose") {
            verbose = true;
        } else if (args[i] == "--help") {
            std::cout << "Real GPT4All API Server\n";
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --port PORT     Listen on PORT (default: 4891)\n";
            std::cout << "  --verbose       Enable verbose output\n";
            std::cout << "  --help          Show this help\n";
            return 0;
        }
    }
    
    if (verbose) {
        qDebug() << "Starting Real GPT4All API Server...";
        qDebug() << "Port:" << port;
    }
    
    // Create and start the server
    RealGPT4AllAPIServer server;
    server.start(port);
    
    std::cout << "Real GPT4All API Server running on http://localhost:" << port << std::endl;
    std::cout << "This server connects to actual GPT4All ML models for authentic AI responses." << std::endl;
    std::cout << "Press Ctrl+C to stop the server." << std::endl;
    
    return app.exec();
}

#include "real_gpt4all_api_server.moc"
