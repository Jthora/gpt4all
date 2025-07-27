#include <QCoreApplication>
#include <QDebug>
#include <QCommandLineParser>    qInfo() << "🚀 GPT4All Local API Server started successfully!";
    qInfo() << "📡 Listening on:" << QString("http://%1:%2").arg(host, QString::number(port));
    qInfo() << "📚 Available endpoints:";
    qInfo() << "   GET /health            - Server health check";
    qInfo() << "   GET /v1/models         - List available models";
    qInfo() << "   POST /v1/chat/completions - Enhanced automotive responses";
    qInfo() << "";
    qInfo() << "💡 Test with: curl http://localhost:" + QString::number(port) + "/health";
    qInfo() << "🚗 Car demo: curl -X POST http://localhost:" + QString::number(port) + "/v1/chat/completions \\";
    qInfo() << "                      -H 'Content-Type: application/json' \\";
    qInfo() << "                      -d '{\"model\": \"gpt4all-test\", \"messages\": [{\"role\": \"user\", \"content\": \"Create a categorized list of all the parts in a car\"}]}'";
    qInfo() << "🛑 Press Ctrl+C to stop the server";ommandLineOption>
#include <QTimer>
#include <csignal>

// Include our Minimal SimpleServer implementation
#include "minimal_simpleserver.h"

MinimalSimpleServer *g_server = nullptr;

void signalHandler(int signal) {
    qDebug() << "Received signal" << signal << "- shutting down gracefully...";
    if (g_server) {
        g_server->stopServer();
    }
    QCoreApplication::quit();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("GPT4All Local API Server");
    app.setApplicationVersion("1.0");
    
    // Set up command line argument parsing
    QCommandLineParser parser;
    parser.setApplicationDescription("GPT4All Local API Server - OpenAI-compatible endpoints");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption portOption(QStringList() << "p" << "port",
                                 "Port to listen on (default: 4891)",
                                 "port", "4891");
    parser.addOption(portOption);
    
    QCommandLineOption hostOption(QStringList() << "host",
                                 "Host to bind to (default: localhost)",
                                 "host", "127.0.0.1");
    parser.addOption(hostOption);
    
    QCommandLineOption verboseOption(QStringList() << "verbose",
                                    "Enable verbose logging");
    parser.addOption(verboseOption);
    
    parser.process(app);
    
    // Get options
    quint16 port = parser.value(portOption).toUShort();
    QString host = parser.value(hostOption);
    bool verbose = parser.isSet(verboseOption);
    
    if (port == 0) {
        qCritical() << "Invalid port number";
        return 1;
    }
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    qInfo() << "Starting GPT4All Local API Server...";
    qInfo() << "Version:" << app.applicationVersion();
    
    // Create the server
    MinimalSimpleServer server;
    g_server = &server;
    
    // Start the server
    if (!server.startServer(port)) {
        qCritical() << "Failed to start server on port" << port;
        return 1;
    }
    
    qInfo() << "🚀 GPT4All Local API Server started successfully!";
    qInfo() << "� REAL AI INTEGRATION: Authentic GPT4All ML responses enabled!";
    qInfo() << "�📡 Listening on:" << QString("http://%1:%2").arg(host, QString::number(port));
    qInfo() << "📚 Available endpoints:";
    qInfo() << "   GET /health            - Server health check";
    qInfo() << "   GET /v1/models         - List available models";
    qInfo() << "   POST /v1/chat/completions - Chat with REAL AI";
    qInfo() << "";
    qInfo() << "💡 Test with: curl http://localhost:" + QString::number(port) + "/health";
    qInfo() << "🤖 Real AI test: curl -X POST http://localhost:" + QString::number(port) + "/v1/chat/completions \\";
    qInfo() << "                      -H 'Content-Type: application/json' \\";
    qInfo() << "                      -d '{\"model\": \"gpt4all\", \"messages\": [{\"role\": \"user\", \"content\": \"Hello!\"}]}'";
    qInfo() << "🛑 Press Ctrl+C to stop the server";
    
    if (verbose) {
        qInfo() << "🔍 Verbose logging enabled";
    }
    
    // Keep the server running until interrupted
    int result = app.exec();
    
    qInfo() << "Server stopped. Goodbye! 👋";
    return result;
}
