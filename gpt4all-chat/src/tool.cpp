#include "tool.h"

#include <QDataStream>
#include <QtGlobal>

#include <string>

using json = nlohmann::ordered_json;


json::object_t Tool::jinjaValue() const
{
    json::array_t paramList;
    const QList<ToolParamInfo> p = parameters();
    for (auto &info : p) {
        std::string typeStr;
        switch (info.type) {
        case ToolEnums::ParamType::String:   typeStr = "string"; break;
        case ToolEnums::ParamType::Number:   typeStr = "number"; break;
        case ToolEnums::ParamType::Integer:  typeStr = "integer"; break;
        case ToolEnums::ParamType::Object:   typeStr = "object"; break;
        case ToolEnums::ParamType::Array:    typeStr = "array"; break;
        case ToolEnums::ParamType::Boolean:  typeStr = "boolean"; break;
        case ToolEnums::ParamType::Null:     typeStr = "null"; break;
        }
        paramList.emplace_back(json::initializer_list_t {
            { "name",        info.name.toStdString()        },
            { "type",        typeStr                        },
            { "description", info.description.toStdString() },
            { "required",    info.required                  },
        });
    }

    return {
        { "name",           name().toStdString()           },
        { "description",    description().toStdString()    },
        { "function",       function().toStdString()       },
        { "parameters",     paramList                      },
        { "symbolicFormat", symbolicFormat().toStdString() },
        { "examplePrompt",  examplePrompt().toStdString()  },
        { "exampleCall",    exampleCall().toStdString()    },
        { "exampleReply",   exampleReply().toStdString()   },
    };
}

void ToolCallInfo::serialize(QDataStream &stream, int version)
{
    stream << name;
    stream << params.size();
    for (auto param : params) {
        stream << param.name;
        stream << param.type;
        stream << param.value;
    }
    stream << result;
    stream << error;
    stream << errorString;
}

bool ToolCallInfo::deserialize(QDataStream &stream, int version)
{
    stream >> name;
    qsizetype count;
    stream >> count;
    for (int i = 0; i < count; ++i) {
        ToolParam p;
        stream >> p.name;
        stream >> p.type;
        stream >> p.value;
    }
    stream >> result;
    stream >> error;
    stream >> errorString;
    return true;
}
