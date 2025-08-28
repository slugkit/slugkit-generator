#include "helpers.hpp"

#include <slugkit/generator/permutations.hpp>

namespace slugkit::generator::benchmarks {

auto GenerateWords(const DictionarySpecs& specs) -> std::vector<Word> {
    std::vector<Word> words;
    for (std::int64_t i = 0; i < specs.size; ++i) {
        WordTags tags;
        for (const auto& tag : specs.tags) {
            auto random_value = Permute(100UL, 0, i);
            if (random_value < tag.probability) {
                tags.insert(tag.tag);
            }
        }
        auto word = specs.name;
        // apply length jitter, if needed
        if (specs.min_length != specs.max_length) {
            auto length = specs.min_length + Permute(specs.max_length - specs.min_length, 0, i);
            while (word.size() < length) {
                word += specs.name;
            }
            word = word.substr(0, length);
        }
        words.push_back({fmt::format("{}_{}", specs.name, i), std::move(tags)});
    }
    return words;
}

auto FillDictionary(const DictionarySpecs& specs) -> Dictionary {
    auto words = GenerateWords(specs);
    return Dictionary{specs.name, specs.language, std::move(words)};
}

auto GenerateSet(const std::vector<DictionarySpecs>& specs) -> DictionarySet {
    std::vector<Dictionary> dictionaries;
    for (const auto& spec : specs) {
        dictionaries.push_back(FillDictionary(spec));
    }
    return DictionarySet{std::move(dictionaries)};
}

}  // namespace slugkit::generator::benchmarks