#include "llmodel_ollama.h"

#include <QCoro/QCoroAsyncGenerator>
#include <QCoro/QCoroTask>

using namespace Qt::Literals::StringLiterals;


namespace gpt4all::ui {


void OllamaGenerationParams::parseInner(QMap<GenerationParam, QVariant> &values)
{
    tryParseValue(values, GenerationParam::NPredict, &OllamaGenerationParams::n_predict);
}

auto OllamaGenerationParams::toMap() const -> QMap<QLatin1StringView, QVariant>
{
    return {
        { "n_predict"_L1, n_predict },
    };
}

OllamaProvider::~OllamaProvider() noexcept = default;

auto OllamaProvider::supportedGenerationParams() const -> QSet<GenerationParam>
{
    using enum GenerationParam;
    return { NPredict };
}

auto OllamaProvider::makeGenerationParams(const QMap<GenerationParam, QVariant> &values) const
    -> OllamaGenerationParams *
{ return new OllamaGenerationParams(values); }

/// load
OllamaProviderCustom::OllamaProviderCustom(ProviderStore *store, QUuid id, QString name, QUrl baseUrl)
    : ModelProvider      (std::move(id), std::move(name), std::move(baseUrl))
    , ModelProviderCustom(store)
{
    if (auto res = m_store->acquire(m_id); !res)
        res.error().raise();
}

/// create
OllamaProviderCustom::OllamaProviderCustom(ProviderStore *store, QString name, QUrl baseUrl)
    : ModelProvider      (std::move(name), std::move(baseUrl))
    , ModelProviderCustom(store)
{
    auto data = m_store->create(m_name, m_baseUrl);
    if (!data)
        data.error().raise();
    m_id = (*data)->id;
}

auto OllamaProviderCustom::asData() -> ModelProviderData
{
    return {
        .id               = m_id,
        .custom_details   = CustomProviderDetails { m_name, m_baseUrl },
        .provider_details = {},
    };
}

OllamaModelDescription::OllamaModelDescription(protected_t, std::shared_ptr<const OllamaProvider> provider,
                                               QByteArray modelHash)
    : m_provider (std::move(provider ))
    , m_modelHash(std::move(modelHash))
    {}

auto OllamaModelDescription::newInstance(QNetworkAccessManager *nam) const -> std::unique_ptr<OllamaChatModel>
{ return std::unique_ptr<OllamaChatModel>(&dynamic_cast<OllamaChatModel &>(*newInstanceImpl(nam))); }

auto OllamaModelDescription::newInstanceImpl(QNetworkAccessManager *nam) const -> ChatLLMInstance *
{ return new OllamaChatModel({ shared_from_this(), this }, nam); }

OllamaChatModel::OllamaChatModel(std::shared_ptr<const OllamaModelDescription> description, QNetworkAccessManager *nam)
    : m_description(std::move(description))
    , m_nam        (nam                   )
    {}

auto OllamaChatModel::preload() -> QCoro::Task<>
{
    // TODO: implement
    co_return;
}

auto OllamaChatModel::generate(QStringView prompt, const GenerationParams *params,
                               /*out*/ ChatResponseMetadata &metadata)
    -> QCoro::AsyncGenerator<QString>
{
    // TODO: implement
    co_yield QStringLiteral("(TODO: response from ollama)");
}


} // namespace gpt4all::ui
