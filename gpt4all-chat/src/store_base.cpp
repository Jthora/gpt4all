#include "store_base.h"

#include <fmt/format.h>
#include <gpt4all-backend/formatters.h> // IWYU pragma: keep

#include <QByteArray>
#include <QDebug>
#include <QIODevice>
#include <QLatin1StringView> // IWYU pragma: keep
#include <QSaveFile>
#include <QUrl>
#include <QtAssert>
#include <QtLogging>

#include <array>
#include <stdexcept>
#include <string>
#include <system_error>

namespace fs   = std::filesystem;
namespace json = boost::json;
namespace sys  = boost::system;
using namespace Qt::StringLiterals;


namespace gpt4all::ui {


DataStoreError::DataStoreError(const QFileDevice *file)
    : m_error(file->error())
    , m_errorString(file->errorString())
{
    Q_ASSERT(file->error());
}

DataStoreError::DataStoreError(const boost::system::system_error &e)
    : m_error(e.code())
    , m_errorString(QString::fromUtf8(e.what()))
{
    Q_ASSERT(e.code());
}

DataStoreError::DataStoreError(QString e)
    : m_error()
    , m_errorString(e)
    {}

void DataStoreError::raise() const
{
    std::visit(Overloaded {
        [&](QFileDevice::FileError    e) { throw FileError(m_errorString, e); },
        [&](boost::system::error_code e) { throw std::runtime_error(m_errorString.toUtf8().constData()); },
        [&](std::monostate             ) { throw std::runtime_error(m_errorString.toUtf8().constData()); },
    }, m_error);
    Q_UNREACHABLE();
}

auto DataStoreBase::reload() -> DataStoreResult<>
{
    if (auto res = clear(); !res)
        return res;

    json::stream_parser parser;
    QFile file;

    for (auto &entry : fs::directory_iterator(m_path)) {
        file.setFileName(entry.path());
        if (!file.open(QFile::ReadOnly)) {
            qWarning().noquote() << "skipping unopenable file:" << file.fileName();
            continue;
        }
        auto jv = read(file, parser);
        if (!jv) {
            (qWarning().nospace() << "skipping " << file.fileName() << "because of read error: ").noquote()
                << jv.error().errorString();
        } else if (auto [unique, uuid] = insert(*jv); !unique)
            qWarning() << "skipping duplicate data store entry:" << uuid;
        file.close();
    }
    return {};
}

auto DataStoreBase::setPath(fs::path path) -> DataStoreResult<>
{
    if (path != m_path) {
        m_path = std::move(path);
        return reload();
    }
    return {};
}

auto DataStoreBase::getFilePath(const QString &name) -> std::filesystem::path
{ return m_path / fmt::format("{}.json", QLatin1StringView(normalizeName(name))); }

auto DataStoreBase::openNew(const QString &name) -> DataStoreResult<std::unique_ptr<QFile>>
{
    auto path = getFilePath(name);
    auto file = std::make_unique<QFile>(path);
    if (file->exists())
        return std::unexpected(sys::system_error(std::make_error_code(std::errc::file_exists), path.string()));
    if (!file->open(QFile::WriteOnly | QFile::NewOnly))
        return std::unexpected(&*file);
    return file;
}

auto DataStoreBase::openExisting(const QString &name) -> DataStoreResult<std::unique_ptr<QSaveFile>>
{
    auto path = getFilePath(name);
    if (!QFile::exists(path))
        return std::unexpected(sys::system_error(
            std::make_error_code(std::errc::no_such_file_or_directory), path.string()
        ));
    auto file = std::make_unique<QSaveFile>(toQString(path));
    if (!file->open(QSaveFile::WriteOnly | QSaveFile::ExistingOnly))
        return std::unexpected(&*file);
    return file;
}

auto DataStoreBase::read(QFileDevice &file, boost::json::stream_parser &parser) -> DataStoreResult<boost::json::value>
{
    for (;;) {
        auto chunk = file.read(JSON_BUFSIZ);
        if (file.error())
            return std::unexpected(&file);
        if (chunk.isEmpty()) {
            Q_ASSERT(file.atEnd());
            break;
        }
        parser.write(chunk.data(), chunk.size());
    }
    return parser.release();
}

auto DataStoreBase::write(const json::value &value, QFileDevice &file) -> DataStoreResult<>
{
    m_serializer.reset(&value);
    std::array<char, JSON_BUFSIZ> buf;
    while (!m_serializer.done()) {
        auto chunk = m_serializer.read(buf.data(), buf.size());
        qint64 nWritten = file.write(chunk.data(), chunk.size());
        if (nWritten < 0)
            return std::unexpected(&file);
        Q_ASSERT(nWritten == chunk.size());
    }

    if (!file.flush())
        return std::unexpected(&file);

    return {};
}

QByteArray DataStoreBase::normalizeName(const QString &name)
{
    auto lower = name.toLower();
    auto norm = QUrl::toPercentEncoding(lower, /*exclude*/ " !#$%&'()+,;=@[]^`{}"_ba, /*include*/ "~"_ba);

    // "." and ".." are special filenames
    return norm == "."_ba  ? "%2E"_ba    :
           norm == ".."_ba ? "%2E%2E"_ba :
           norm;
}


} // namespace gpt4all::ui
