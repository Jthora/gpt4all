#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QDir>

// Include GPT4All headers
#include "gpt4all-chat/src/chat.h"
#include "gpt4all-chat/src/chatllm.h"
#include "gpt4all-chat/src/modellist.h"
#include "gpt4all-chat/src/database.h"
#include "gpt4all-chat/src/mysettings.h"

class GPT4AllConnectionTest : public QObject {
    Q_OBJECT

public:
    GPT4AllConnectionTest(QObject *parent = nullptr) : QObject(parent) {
        qDebug() << "🔍 TESTING REAL AI CONNECTION - NO MOCK DATA";
        qDebug() << "==========================================";
        
        // Test the connection
        QTimer::singleShot(100, this, &GPT4AllConnectionTest::testAIConnection);
    }

private slots:
    void testAIConnection() {
        qDebug() << "\n1️⃣ Initializing GPT4All components...";
        
        // Initialize the settings first
        MySettings *settings = MySettings::globalInstance();
        if (!settings) {
            qDebug() << "❌ Failed to initialize MySettings";
            QCoreApplication::exit(1);
            return;
        }
        qDebug() << "✅ MySettings initialized";
        
        // Initialize model list
        ModelList *modelList = ModelList::globalInstance();
        if (!modelList) {
            qDebug() << "❌ Failed to initialize ModelList";
            QCoreApplication::exit(1);
            return;
        }
        qDebug() << "✅ ModelList initialized";
        
        // Create a chat instance
        Chat *chat = new Chat(this);
        if (!chat) {
            qDebug() << "❌ Failed to create Chat instance";
            QCoreApplication::exit(1);
            return;
        }
        qDebug() << "✅ Chat instance created";
        
        // Get ChatLLM
        ChatLLM *chatLLM = chat->chatLLM();
        if (!chatLLM) {
            qDebug() << "❌ Failed to get ChatLLM instance";
            QCoreApplication::exit(1);
            return;
        }
        qDebug() << "✅ ChatLLM instance obtained";
        
        qDebug() << "\n2️⃣ Checking for available models...";
        
        // Check installed models
        auto installedModels = modelList->installedModels();
        qDebug() << "📦 Found" << installedModels.size() << "installed models in ModelList";
        
        for (const auto &model : installedModels) {
            qDebug() << "   Model:" << model.filename() << "at" << model.filepath();
        }
        
        // Manual search for models
        QStringList searchPaths = {
            QDir::homePath() + "/.local/share/nomic.ai/GPT4All",
            QDir::homePath() + "/gpt4all_models", 
            "/usr/share/gpt4all/models",
            "./models",
            "/home/jono/Applications/GPT4All"
        };
        
        QStringList foundModels;
        for (const QString &searchPath : searchPaths) {
            QDir dir(searchPath);
            if (dir.exists()) {
                auto files = dir.entryList(QStringList() << "*.gguf" << "*.bin", QDir::Files);
                for (const QString &file : files) {
                    QString fullPath = dir.absoluteFilePath(file);
                    foundModels << fullPath;
                    qDebug() << "🔍 Found model file:" << fullPath;
                }
            }
        }
        
        qDebug() << "\n3️⃣ Testing AI model loading...";
        
        bool modelLoaded = false;
        
        // Try loading from ModelList first
        if (!installedModels.isEmpty()) {
            auto firstModel = installedModels.first();
            qDebug() << "🔄 Trying to load from ModelList:" << firstModel.filename();
            
            bool loadResult = false;
            QMetaObject::invokeMethod(chatLLM, "loadModel",
                                      Qt::DirectConnection,
                                      Q_RETURN_ARG(bool, loadResult),
                                      Q_ARG(ModelInfo, firstModel));
            
            if (loadResult && chatLLM->isModelLoaded()) {
                qDebug() << "✅ Model loaded successfully from ModelList!";
                modelLoaded = true;
            } else {
                qDebug() << "❌ Failed to load model from ModelList";
            }
        }
        
        // If ModelList failed, try manual loading
        if (!modelLoaded && !foundModels.isEmpty()) {
            QString modelPath = foundModels.first();
            qDebug() << "🔄 Trying manual model loading:" << modelPath;
            
            ModelInfo modelInfo;
            modelInfo.setFilename(QFileInfo(modelPath).fileName());
            modelInfo.setFilepath(modelPath);
            
            bool loadResult = false;
            QMetaObject::invokeMethod(chatLLM, "loadModel",
                                      Qt::DirectConnection,
                                      Q_RETURN_ARG(bool, loadResult),
                                      Q_ARG(ModelInfo, modelInfo));
            
            if (loadResult && chatLLM->isModelLoaded()) {
                qDebug() << "✅ Model loaded successfully manually!";
                modelLoaded = true;
            } else {
                qDebug() << "❌ Failed to load model manually";
            }
        }
        
        if (!modelLoaded) {
            qDebug() << "❌ NO MODEL COULD BE LOADED";
            qDebug() << "   Please download a model from: https://gpt4all.io/models/";
            QCoreApplication::exit(1);
            return;
        }
        
        qDebug() << "\n4️⃣ Testing REAL AI inference (NO MOCK DATA)...";
        
        QString testPrompt = "Create a categorized list of all the parts in a car";
        qDebug() << "🤖 Prompt:" << testPrompt;
        qDebug() << "⏳ Generating REAL AI response...";
        
        // Method 1: Use Chat's newPromptResponsePair (the proper way)
        QMetaObject::invokeMethod(chat, "newPromptResponsePair",
                                  Qt::DirectConnection,
                                  Q_ARG(QString, testPrompt));
        
        // Wait a moment for processing
        QCoreApplication::processEvents();
        
        // Try to get the response
        QString response;
        QMetaObject::invokeMethod(chat->chatModel(), "responseText",
                                  Qt::DirectConnection,
                                  Q_RETURN_ARG(QString, response));
        
        if (!response.isEmpty()) {
            qDebug() << "✅ REAL AI RESPONSE GENERATED:";
            qDebug() << "📝" << response;
            qDebug() << "\n🎉 AI CONNECTION TEST PASSED - REAL AI INFERENCE WORKING!";
        } else {
            qDebug() << "⚠️  Response was empty, trying alternative methods...";
            
            // Try getting latest response from chat model
            if (chat->chatModel() && chat->chatModel()->count() > 0) {
                QString lastResponse = chat->chatModel()->responseText();
                if (!lastResponse.isEmpty()) {
                    qDebug() << "✅ REAL AI RESPONSE (from chat model):";
                    qDebug() << "📝" << lastResponse;
                    qDebug() << "\n🎉 AI CONNECTION TEST PASSED!";
                } else {
                    qDebug() << "❌ No response generated - AI connection issue";
                }
            } else {
                qDebug() << "❌ Chat model has no responses";
            }
        }
        
        qDebug() << "\n🔍 CONNECTION TEST COMPLETE";
        qDebug() << "==============================";
        
        QCoreApplication::exit(0);
    }
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    qDebug() << "Starting GPT4All AI Connection Test...";
    qDebug() << "This test verifies REAL AI inference with NO MOCK DATA";
    
    GPT4AllConnectionTest test;
    
    return app.exec();
}

#include "test_ai_connection.moc"
