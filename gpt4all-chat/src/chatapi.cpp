#include "chatapi.h"

#include "utils.h"

#include <fmt/format.h>

#include <QAnyStringView>
#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLatin1String>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QStringView>
#include <QThread>
#include <QUrl>
#include <QUtf8StringView> // IWYU pragma: keep
#include <QVariant>
#include <QXmlStreamReader>
#include <Qt>
#include <QDebug> // Qt 6.2 compatibility
#include <QLoggingCategory>

#include <expected>
#include <functional>
#include <iostream>
#include <utility>

//#define DEBUG


ChatAPI::ChatAPI()
    : QObject(nullptr)
    , m_modelName("gpt-3.5-turbo")
    , m_requestURL("")
    , m_responseCallback(nullptr)
{
}

size_t ChatAPI::requiredMem(const std::string &modelPath, int n_ctx, int ngl)
{
    Q_UNUSED(modelPath);
    Q_UNUSED(n_ctx);
    Q_UNUSED(ngl);
    return 0;
}

bool ChatAPI::loadModel(const std::string &modelPath, int n_ctx, int ngl)
{
    Q_UNUSED(modelPath);
    Q_UNUSED(n_ctx);
    Q_UNUSED(ngl);
    return true;
}

void ChatAPI::setThreadCount(int32_t n_threads)
{
    Q_UNUSED(n_threads);
}

int32_t ChatAPI::threadCount() const
{
    return 1;
}

ChatAPI::~ChatAPI()
{
}

bool ChatAPI::isModelLoaded() const
{
    return true;
}

static auto parsePrompt(QXmlStreamReader &xml) -> std::expected<QJsonArray, QString>
{
    QJsonArray messages;

    auto xmlError = [&xml] {
        return std::unexpected(QString("%1:%2: %3").arg(xml.lineNumber()).arg(xml.columnNumber()).arg(xml.errorString()));
    };

    if (xml.hasError())
        return xmlError();
    if (xml.atEnd())
        return messages;

    // skip header
    bool foundElement = false;
    do {
        switch (xml.readNext()) {
        case QXmlStreamReader::Invalid:
            return xmlError();
        case QXmlStreamReader::EndDocument:
            return messages;
        default:
            foundElement = true;
        case QXmlStreamReader::StartDocument:
        case QXmlStreamReader::Comment:
        case QXmlStreamReader::DTD:
        case QXmlStreamReader::ProcessingInstruction:
            ;
        }
    } while (!foundElement);

    // document body loop
    bool foundRoot = false;
    for (;;) {
        switch (xml.tokenType()) {
        case QXmlStreamReader::StartElement:
            {
                auto name = xml.name();
                if (!foundRoot) {
                    if (name != QLatin1String("chat"))
                        return std::unexpected(QString("unexpected tag: %1").arg(name));
                    foundRoot = true;
                } else {
                    if (name != QLatin1String("user") && name != QLatin1String("assistant") && name != QLatin1String("system"))
                        return std::unexpected(QString("unknown role: %1").arg(name));
                    auto content = xml.readElementText();
                    if (xml.tokenType() != QXmlStreamReader::EndElement)
                        return xmlError();
                    messages << makeJsonObject({
                        { QLatin1String("role"),    name.toString().trimmed() },
                        { QLatin1String("content"), content                   },
                    });
                }
                break;
            }
        case QXmlStreamReader::Characters:
            if (!xml.isWhitespace())
                return std::unexpected(QString("unexpected text: %1").arg(xml.text()));
        case QXmlStreamReader::Comment:
        case QXmlStreamReader::ProcessingInstruction:
        case QXmlStreamReader::EndElement:
            break;
        case QXmlStreamReader::EndDocument:
            return messages;
        case QXmlStreamReader::Invalid:
            return xmlError();
        default:
            return std::unexpected(QString("unexpected token: %1").arg(xml.tokenString()));
        }
        xml.readNext();
    }
}

void ChatAPI::prompt(
    std::string_view        prompt,
    const PromptCallback   &promptCallback,
    const ResponseCallback &responseCallback,
    const PromptContext    &promptCtx
) {
    Q_UNUSED(promptCallback)

    if (!isModelLoaded())
        throw std::invalid_argument("Attempted to prompt an unloaded model.");
    if (!promptCtx.n_predict)
        return; // nothing requested

    // FIXME: We don't set the max_tokens on purpose because in order to do so safely without encountering
    // an error we need to be able to count the tokens in our prompt. The only way to do this is to use
    // the OpenAI tiktoken library or to implement our own tokenization function that matches precisely
    // the tokenization used by the OpenAI model we're calling. OpenAI has not introduced any means of
    // using the REST API to count tokens in a prompt.
    auto root = makeJsonObject({
        { QLatin1String("model"),       m_modelName     },
        { QLatin1String("stream"),      true            },
        { QLatin1String("temperature"), promptCtx.temp  },
        { QLatin1String("top_p"),       promptCtx.top_p },
    });

    // conversation history
    {
        QString promptStr = QString::fromUtf8(prompt.data(), prompt.size());
        QXmlStreamReader xml(promptStr);
        auto messages = parsePrompt(xml);
        if (!messages) {
            auto error = fmt::format("Failed to parse API model prompt: {}", messages.error());
            qDebug().noquote() << "ChatAPI ERROR:" << QString::fromStdString(error) << "Prompt:\n\n" << promptStr << '\n';
            throw std::invalid_argument(error);
        }
        root.insert(QLatin1String("messages"), *messages);
    }

    QJsonDocument doc(root);

#if defined(DEBUG)
    qDebug().noquote() << "ChatAPI::prompt begin network request" << doc.toJson();
#endif

    m_responseCallback = responseCallback;

    // The following code sets up a worker thread and object to perform the actual api request to
    // chatgpt and then blocks until it is finished
    QThread workerThread;
    ChatAPIWorker worker(this);
    worker.moveToThread(&workerThread);
    connect(&worker, &ChatAPIWorker::finished, &workerThread, &QThread::quit, Qt::DirectConnection);
    connect(this, &ChatAPI::request, &worker, &ChatAPIWorker::request, Qt::QueuedConnection);
    workerThread.start();
    emit request(m_apiKey, doc.toJson(QJsonDocument::Compact));
    workerThread.wait();

    m_responseCallback = nullptr;

#if defined(DEBUG)
    qDebug() << "ChatAPI::prompt end network request";
#endif
}

bool ChatAPI::callResponse(int32_t token, const std::string& string)
{
    Q_ASSERT(m_responseCallback);
    if (!m_responseCallback) {
        std::cerr << "ChatAPI ERROR: no response callback!\n";
        return false;
    }
    return m_responseCallback(token, string);
}

void ChatAPIWorker::request(const QString &apiKey, const QByteArray &array)
{
    QUrl apiUrl(m_chat->url());
    const QString authorization = QString("Bearer %1").arg(apiKey).trimmed();
    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", authorization.toUtf8());
#if defined(DEBUG)
    qDebug() << "ChatAPI::request"
             << "API URL: " << apiUrl.toString()
             << "Authorization: " << authorization.toUtf8();
#endif
    m_networkManager = new QNetworkAccessManager(this);
    QNetworkReply *reply = m_networkManager->post(request, array);
    connect(qGuiApp, &QCoreApplication::aboutToQuit, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, &ChatAPIWorker::handleFinished);
    connect(reply, &QNetworkReply::readyRead, this, &ChatAPIWorker::handleReadyRead);
    connect(reply, &QNetworkReply::errorOccurred, this, &ChatAPIWorker::handleErrorOccurred);
}

void ChatAPIWorker::handleFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        emit finished();
        return;
    }

    QVariant response = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    if (!response.isValid()) {
        m_chat->callResponse(
            -1,
            tr("ERROR: Network error occurred while connecting to the API server")
                .toStdString()
        );
        return;
    }

    bool ok;
    int code = response.toInt(&ok);
    if (!ok || code != 200) {
        bool isReplyEmpty(reply->readAll().isEmpty());
        if (isReplyEmpty)
            m_chat->callResponse(
                -1,
                tr("ChatAPIWorker::handleFinished got HTTP Error %1 %2")
                    .arg(code)
                    .arg(reply->errorString())
                    .toStdString()
            );
        qWarning().noquote() << "ERROR: ChatAPIWorker::handleFinished got HTTP Error" << code << "response:"
                             << reply->errorString();
    }
    reply->deleteLater();
    emit finished();
}

void ChatAPIWorker::handleReadyRead()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        emit finished();
        return;
    }

    QVariant response = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    if (!response.isValid())
        return;

    bool ok;
    int code = response.toInt(&ok);
    if (!ok || code != 200) {
        m_chat->callResponse(
            -1,
            QString("ERROR: ChatAPIWorker::handleReadyRead got HTTP Error %1 %2: %3")
                .arg(code).arg(reply->errorString(), reply->readAll()).toStdString()
        );
        emit finished();
        return;
    }

    while (reply->canReadLine()) {
        QString jsonData = reply->readLine().trimmed();
        if (jsonData.startsWith("data:"))
            jsonData.remove(0, 5);
        jsonData = jsonData.trimmed();
        if (jsonData.isEmpty())
            continue;
        if (jsonData == "[DONE]")
            continue;
#if defined(DEBUG)
        qDebug().noquote() << "line" << jsonData;
#endif
        QJsonParseError err;
        const QJsonDocument document = QJsonDocument::fromJson(jsonData.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            m_chat->callResponse(-1, QString("ERROR: ChatAPI responded with invalid json \"%1\"")
                                         .arg(err.errorString()).toStdString());
            continue;
        }

        const QJsonObject root = document.object();
        const QJsonArray choices = root.value("choices").toArray();
        const QJsonObject choice = choices.first().toObject();
        const QJsonObject delta = choice.value("delta").toObject();
        const QString content = delta.value("content").toString();
        m_currentResponse += content;
        if (!m_chat->callResponse(0, content.toStdString())) {
            reply->abort();
            emit finished();
            return;
        }
    }
}

void ChatAPIWorker::handleErrorOccurred(QNetworkReply::NetworkError code)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply || reply->error() == QNetworkReply::OperationCanceledError /*when we call abort on purpose*/) {
        emit finished();
        return;
    }

    qWarning().noquote() << "ERROR: ChatAPIWorker::handleErrorOccurred got HTTP Error" << code << "response:"
                         << reply->errorString();
    emit finished();
}
