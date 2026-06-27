#include "test_dictionary.hpp"

#include <generated/test-adv.slugs.hpp>
#include <generated/emoji.bin.hpp>

#include <cstdint>
#include <map>
#include <string>

namespace slugkit::generator::test {

const std::span<const std::byte> kDictionaryTestData = std::span<const std::byte>(
    reinterpret_cast<const std::byte*>(dictionary_test_data_begin),
    dictionary_test_data_size
);

const std::span<const std::byte> kEmojiTestData = std::span<const std::byte>(
    reinterpret_cast<const std::byte*>(emoji_test_data_begin),
    emoji_test_data_size
);

std::vector<Dictionary> Decompile(const binary::BinaryDictionary& dict) {
    const std::string kind{dict.Kind()};

    std::map<std::uint32_t, WordTags> word_tags;
    for (const auto& tag_view : dict.Tags()) {
        Tag tag{std::string{tag_view.GetUnderlying()}};
        const auto& entry = dict[tag_view];
        for (auto it = entry.begin(); it != entry.end(); ++it) {
            word_tags[it->GetUnderlying()].insert(tag);
        }
    }

    std::vector<Dictionary> dictionaries;
    for (const auto& lang : dict.Languages()) {
        const auto& info = dict[lang];
        const auto start = info.FullRange().front().GetUnderlying();
        const auto count = info.Count().GetUnderlying();
        std::vector<Word> words;
        words.reserve(count);
        for (std::uint32_t i = start; i < start + count; ++i) {
            Word w;
            w.word = std::string{dict[IndexType(i)].Lowercase()};
            auto f = word_tags.find(i);
            if (f != word_tags.end()) {
                w.tags = f->second;
            }
            words.push_back(std::move(w));
        }
        dictionaries.emplace_back(kind, lang, std::move(words));
    }
    return dictionaries;
}

}  // namespace slugkit::generator::test
