#pragma once

#include <slugkit/generator/dictionary.hpp>

namespace slugkit::generator::benchmarks {

struct TagProbability {
    std::string tag;
    std::uint64_t probability;  // 0-100
};

struct DictionarySpecs {
    std::string name;
    std::string language;
    std::int64_t size;
    std::vector<TagProbability> tags;
    std::size_t min_length;
    std::size_t max_length;
};

auto GenerateWords(const DictionarySpecs& specs) -> std::vector<Word>;

auto FillDictionary(const DictionarySpecs& specs) -> Dictionary;

auto GenerateSet(const std::vector<DictionarySpecs>& specs) -> DictionarySet;

}  // namespace slugkit::generator::benchmarks