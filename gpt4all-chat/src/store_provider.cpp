#include "store_provider.h"

#include "json-helpers.h" // IWYU pragma: keep

#include <gpt4all-backend/json-helpers.h> // IWYU pragma: keep

#include <utility>

namespace json = boost::json;


namespace gpt4all::ui {


void tag_invoke(const boost::json::value_from_tag &, boost::json::value &jv, ModelProviderData data)
{
    auto &obj = jv.emplace_object();
    obj = { { "id",      data.id              },
            { "builtin", !data.custom_details },
            { "type",    data.type()          } };
    if (auto custom = data.custom_details) {
        obj.emplace("name",     custom->name);
        obj.emplace("base_url", custom->base_url);
    }
    switch (data.type()) {
        using enum ProviderType;
    case openai:
        obj.emplace("api_key", std::get<size_t(openai)>(data.provider_details).api_key);
    case ollama:
        ;
    }
}

auto tag_invoke(const boost::json::value_to_tag<ModelProviderData> &, const boost::json::value &jv)
    -> ModelProviderData
{
    auto &obj = jv.as_object();
    auto type = json::value_to<ProviderType>(jv.at("type"));
    std::optional<CustomProviderDetails> custom_details;
    if (!jv.at("builtin").as_bool())
        custom_details.emplace(CustomProviderDetails {
            json::value_to<QString>(jv.at("name"    )),
            json::value_to<QString>(jv.at("base_url")),
        });
    ModelProviderData::ProviderDetails provider_details;
    switch (type) {
        using enum ProviderType;
    case openai:
        provider_details = OpenaiProviderDetails { json::value_to<QString>(jv.at("api_key")) };
    case ollama:
        ;
    }
    return {
        .id               = json::value_to<QUuid>(obj.at("id")),
        .custom_details   = std::move(custom_details),
        .provider_details = std::move(provider_details)
    };
}

auto ProviderStore::create(QString name, QUrl base_url, QString api_key)
    -> DataStoreResult<const ModelProviderData *>
{
    ModelProviderData data {
        .id               = QUuid::createUuid(),
        .custom_details   = CustomProviderDetails { name, std::move(base_url) },
        .provider_details = OpenaiProviderDetails { std::move(api_key)        },
    };
    return createImpl(std::move(data), name);
}

auto ProviderStore::create(QString name, QUrl base_url)
    -> DataStoreResult<const ModelProviderData *>
{
    ModelProviderData data {
        .id               = QUuid::createUuid(),
        .custom_details   = CustomProviderDetails { name, std::move(base_url) },
        .provider_details = {},
    };
    return createImpl(std::move(data), name);
}


} // namespace gpt4all::ui
