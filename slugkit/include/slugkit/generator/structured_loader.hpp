#pragma once

#include <slugkit/generator/dictionary.hpp>
#include <slugkit/generator/io/types_io.hpp>
#include <slugkit/generator/types.hpp>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/utils/strong_typedef.hpp>

#include <string>
#include <unordered_set>
#include <vector>

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
    dictionary.words = value["words"].template As<std::vector<BasicWord<TagsType>>>();
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

}  // namespace slugkit::generator