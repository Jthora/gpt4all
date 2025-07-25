#ifndef TOOLCALLPARSER_H
#define TOOLCALLPARSER_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList> // IWYU pragma: keep

namespace ToolEnums { enum class ParseState; }

// Qt 6.2 compatibility - Qt::Literals not available


class ToolCallParser
{
public:
    ToolCallParser();
    ToolCallParser(const QStringList &tagNames);

    void reset();
    void update(const QByteArray &update);
    QString toolCall() const { return QString::fromUtf8(m_toolCall); }
    int startIndex() const { return m_startIndex; }
    ToolEnums::ParseState state() const { return m_state; }
    QByteArray startTag() const;
    QByteArray endTag() const;

    bool splitIfPossible();
    QStringList buffers() const;
    int numberOfBuffers() const { return m_buffers.size(); }

    static QString makeStartTag(const QString &name) { return QString("<%1>").arg(name); }
    static QString makeEndTag  (const QString &name) { return QString("</%1>").arg(name); }

private:
    QByteArray &currentBuffer();
    void resetSearchState();
    bool isExpected(char c) const;
    void setExpected(const QList<QByteArray> &tags);

    QList<QByteArray> m_possibleStartTags;
    QList<QByteArray> m_possibleEndTags;
    QByteArray m_startTagBuffer;
    QByteArray m_endTagBuffer;
    int m_currentTagIndex;

    QList<char> m_expected;
    int m_expectedIndex;
    ToolEnums::ParseState m_state;
    QList<QByteArray> m_buffers;
    QByteArray m_toolCall;
    int m_startIndex;
    int m_endIndex;
};

namespace ToolCallConstants
{
    // NB: the parsing code assumes the first char of the various tags differ

    inline const QString CodeInterpreterFunction = QString("javascript_interpret");
    inline const QString CodeInterpreterStartTag = ToolCallParser::makeStartTag(CodeInterpreterFunction);
    inline const QString CodeInterpreterEndTag   = ToolCallParser::makeEndTag  (CodeInterpreterFunction);
    inline const QString CodeInterpreterPrefix   = QString("%1\n```javascript\n").arg(CodeInterpreterStartTag);
    inline const QString CodeInterpreterSuffix   = QString("```\n%1")            .arg(CodeInterpreterEndTag  );

    inline const QString ThinkTagName  = QString("think");
    inline const QString ThinkStartTag = ToolCallParser::makeStartTag(ThinkTagName);
    inline const QString ThinkEndTag   = ToolCallParser::makeEndTag  (ThinkTagName);

    inline const QStringList AllTagNames { CodeInterpreterFunction, ThinkTagName };
}

#endif // TOOLCALLPARSER_H
