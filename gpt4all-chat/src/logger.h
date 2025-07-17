#ifndef LOGGER_H
#define LOGGER_H

#include <QFile>
#include <QMutex>
#include <QString>
#include <QLoggingCategory> // Qt 6.2 compatibility (QtLogging header doesn't exist)


class Logger {
public:
    explicit Logger();

    static Logger *globalInstance();

private:
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

private:
    QFile  m_file;
    QMutex m_mutex;

    friend class MyLogger;
};

#endif // LOGGER_H
