#include <slugkit/generator/structured_loader.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/yaml.hpp>

#include <userver/utest/utest.hpp>

namespace slugkit::generator::tests {

namespace {

using namespace userver::formats::literals;

const auto kYamlDictionary = R"(
adjective:
  language: en
  words:
    big: [size]
    small: [size]
    beautiful: []
    ugly: []
    red: [color]
    blue: [color]
    green: [color]
noun:
  language: en
  words:
    apple: [fruit]
    banana: [fruit]
    orange: [fruit]
verb:
  language: en
  words:
    run: [action]
    jump: [action]
    walk: [action]
    swim: [action]
)"_yaml;

const auto kJsonDictionary = R"(
{
    "adjective": {
        "language": "en",
        "words": {
            "big": ["size"],
            "small": ["size"]
        }
    },
    "noun": {
        "language": "en",
        "words": {
            "apple": ["fruit"]
        }
    },
    "verb": {
        "language": "en",
        "words": {
            "run": ["action"]
        }
    }
})"_json;

}  // namespace

UTEST(StructuredLoader, EmptyJson) {
    auto dictionary_set = DictionarySet::Parse(R"({})"_json);
    EXPECT_EQ(dictionary_set.size(), 0);
}

UTEST(StructuredLoader, EmptyYaml) {
    auto dictionary_set = DictionarySet::Parse(R"({})"_yaml);
    EXPECT_EQ(dictionary_set.size(), 0);
}

UTEST(StructuredLoader, YamlDictionary) {
    auto dictionary_set = DictionarySet::Parse(kYamlDictionary);
    EXPECT_EQ(dictionary_set.size(), 3);
}

UTEST(StructuredLoader, YamlDictionarySting) {
    auto dictionary_set = DictionarySet::Parse<userver::formats::yaml::Value>(ToString(kYamlDictionary));
    EXPECT_EQ(dictionary_set.size(), 3);
}

UTEST(StructuredLoader, JsonDictionary) {
    auto dictionary_set = DictionarySet::Parse(kJsonDictionary);
    EXPECT_EQ(dictionary_set.size(), 3);
}

UTEST(StructuredLoader, JsonDictionarySting) {
    auto dictionary_set = DictionarySet::Parse<userver::formats::json::Value>(ToString(kJsonDictionary));
    EXPECT_EQ(dictionary_set.size(), 3);
}

}  // namespace slugkit::generator::tests
