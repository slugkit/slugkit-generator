#include "helpers.hpp"

#include <slugkit/generator/permutations.hpp>

namespace slugkit::generator::benchmarks {

auto FillDictionary(const DictionarySpecs& specs) -> Dictionary {
    std::vector<Word> words;
    for (std::int64_t i = 0; i < specs.size; ++i) {
        WordTags tags;
        for (const auto& tag : specs.tags) {
            auto random_value = Permute(100UL, 0, i);
            if (random_value < tag.probability) {
                tags.insert(tag.tag);
            }
        }
        words.push_back({fmt::format("{}_{}", specs.name, i), std::move(tags)});
    }
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