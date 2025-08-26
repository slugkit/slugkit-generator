#pragma once

#include <slugkit/generator/dictionary_types.hpp>
#include <slugkit/generator/types.hpp>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/formats/serialize/to.hpp>

#include <userver/storages/postgres/io/enum_types.hpp>
#include <userver/storages/postgres/io/uuid.hpp>
#include <userver/utils/trivial_map.hpp>

namespace slugkit::generator {

template <typename Format>
auto Serialize(const CaseType& case_type, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder(ToString(case_type));
    return builder.ExtractValue();
}

template <typename Format, typename TagsType>
auto Serialize(const BasicWord<TagsType>& word, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder;
    builder["word"] = word.word;
    builder["kind"] = word.kind;
    builder["language"] = word.language;
    builder["tags"] = word.tags;
    return builder.ExtractValue();
}

template <typename Value, typename TagsType>
auto Parse(const Value& value, userver::formats::parse::To<BasicWord<TagsType>>) -> BasicWord<TagsType> {
    BasicWord<TagsType> word;
    word.word = value["word"].template As<std::string>();
    if (value.HasMember("kind")) {
        word.kind = value["kind"].template As<std::string>();
    }
    if (value.HasMember("language")) {
        word.language = value["language"].template As<std::string>();
    }
    if (value.HasMember("tags")) {
        word.tags = value["tags"].template As<TagsType>();
    }
    return word;
}

// ---------------------------------------------------------------------------------------------------------------------
template <typename Format>
auto Serialize(const DictionaryStats& stats, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder;
    builder["kind"] = stats.kind;
    builder["language"] = stats.language;
    builder["count"] = stats.count;
    return builder.ExtractValue();
}

template <typename Format>
auto Serialize(const TagDefinition& tag, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder;
    builder["kind"] = tag.kind;
    builder["tag"] = tag.tag;
    if (tag.description.has_value()) {
        builder["description"] = tag.description.value();
    }
    builder["opt_in"] = tag.opt_in;
    builder["word_count"] = tag.word_count;

    return builder.ExtractValue();
}

template <typename Format>
auto Serialize(const SelectorSettings& settings, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder;
    builder["original_size"] = settings.original_size;
    builder["selected_size"] = settings.selected_size;
    return builder.ExtractValue();
}

template <typename Value>
auto Parse(const Value& value, userver::formats::parse::To<SelectorSettings>) -> SelectorSettings {
    SelectorSettings result;
    result.original_size = value["original_size"].template As<std::int64_t>();
    result.selected_size = value["selected_size"].template As<std::int64_t>();
    return result;
}

}  // namespace slugkit::generator
