#pragma once

#include <slugkit/generator/dictionary.hpp>
#include <slugkit/generator/io/types_io.hpp>
#include <slugkit/generator/types.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/yaml.hpp>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/formats/serialize/to.hpp>

#include <userver/utils/strong_typedef.hpp>

#include <iosfwd>
#include <string>
#include <unordered_set>
#include <vector>

namespace userver::formats {

template <typename Value>
struct Deserialize;

template <>
struct Deserialize<json::Value> {
    static auto FromString(const std::string& value) -> json::Value {
        return json::FromString(value);
    }

    static auto FromStream(std::istream& stream) -> json::Value {
        return json::FromStream(stream);
    }
};

template <>
struct Deserialize<yaml::Value> {
    static auto FromString(const std::string& value) -> yaml::Value {
        return yaml::FromString(value);
    }

    static auto FromStream(std::istream& stream) -> yaml::Value {
        return yaml::FromStream(stream);
    }
};

}  // namespace userver::formats

namespace slugkit::generator {

namespace data {

/// @brief A dictionary with words and tags.
/// This is an auxilary type for the structured loader.
template <typename TagsType = std::unordered_set<std::string>>
struct Dictionary {
    std::string name;
    std::string language;
    std::vector<BasicWord<TagsType>> words;
};

template <typename Value, typename TagsType>
auto Parse(const Value& value, userver::formats::parse::To<Dictionary<TagsType>>) -> Dictionary<TagsType> {
    Dictionary<TagsType> dictionary;
    if (value.HasMember("name")) {
        dictionary.name = value["name"].template As<std::string>();
    }
    if (value.HasMember("language")) {
        dictionary.language = value["language"].template As<std::string>();
    }
    // words are mandatory
    if (!value.HasMember("words")) {
        throw std::runtime_error("Expected a 'words' field");
    }
    if (!value["words"].IsObject()) {
        throw std::runtime_error("Expected a 'words' field to be an object");
    }
    for (auto item = value["words"].begin(); item != value["words"].end(); ++item) {
        auto tags = item->template As<TagsType>();
        dictionary.words.emplace_back(item.GetName(), std::move(tags));
    }
    return dictionary;
}

}  // namespace data

template <typename Value>
auto Parse(const Value& value, userver::formats::parse::To<DictionarySet>) -> DictionarySet {
    std::vector<Dictionary> dictionaries;
    if (!value.IsObject()) {
        throw std::runtime_error("Expected an object");
    }
    for (auto item = value.begin(); item != value.end(); ++item) {
        auto key = item.GetName();
        auto dictionary = item->template As<data::Dictionary<std::unordered_set<std::string>>>();
        dictionaries.emplace_back(key, dictionary.language, std::move(dictionary.words));
    }
    return DictionarySet(std::move(dictionaries));
}

template <typename Value>
auto DictionarySet::Parse(const std::string& data) -> DictionarySet {
    return DictionarySet::Parse(userver::formats::Deserialize<Value>::FromString(data));
}

template <typename Value>
auto DictionarySet::Parse(std::istream& stream) -> DictionarySet {
    return DictionarySet::Parse(userver::formats::Deserialize<Value>::FromStream(stream));
}

template <typename Value>
auto DictionarySet::Parse(const Value& value) -> DictionarySet {
    return value.template As<DictionarySet>();
}

}  // namespace slugkit::generator
