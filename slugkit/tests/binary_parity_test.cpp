// Generation parity: a PatternGenerator fed a binary (memory-mapped) dictionary set must
// produce byte-identical slugs to one fed the equivalent in-memory dictionary set. The
// binary set is decompiled into an in-memory set so both carry identical data, then the two
// generators are compared across selector shapes, cases, seeds and sequence numbers.
#include <slugkit/generator/binary_dictionary.hpp>
#include <slugkit/generator/dictionary.hpp>
#include <slugkit/generator/pattern.hpp>
#include <slugkit/generator/pattern_generator.hpp>

#include <slugkit/test_utils/test_dictionary.hpp>
#include <slugkit/test_utils/data_config.hpp>

#include <slugkit/utils/memory_mapped_file.hpp>

#include <userver/utest/utest.hpp>

#include <map>
#include <string>
#include <vector>

namespace slugkit::generator {

namespace {

// Decompile a BinaryDictionary into one in-memory Dictionary per language, carrying the same
// words and per-word tags, so an in-memory DictionarySet can be built from identical data.
auto BuildInMemorySet(const binary::BinaryDictionary& dict) -> DictionarySet {
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
    return DictionarySet{std::move(dictionaries)};
}

// The case of a selector is derived from the kind's letter casing (Selector::GetCase):
// lower=adverb, upper=ADVERB, title=Adverb, mixed=AdVerb.
const std::vector<std::string> kPatterns = {
    "{adverb}",
    "{adverb@en}",
    "{adverb@fr}",
    "{adverb:<10}",
    "{adverb@en:<8}",
    "{adverb:>=12}",
    "{adverb:+det}",
    "{adverb:+det-nsfw}",
    "{adverb@en:+obj}",
    "{adverb:+det-nsfw<10}",
    "{adverb@fr:+neut>=9}",
    "{ADVERB}",
    "{ADVERB@en:+obj}",
    "{Adverb}",
    "{Adverb@fr:<11}",
    "{AdVerb}",
    "{adverb}-{adverb@fr}-{number:3d}",
    "{Adverb@en:+det}_{adverb@fr}~{special:2}",
};

void TestSlugParity(std::span<const std::byte> data, std::shared_ptr<void> keepalive) {
    binary::BinaryDictionary dict(data);
    auto mem = BuildInMemorySet(dict);

    binary::DictionarySet bin;
    bin.Add(data, std::move(keepalive));

    for (const auto& pattern_str : kPatterns) {
        auto pattern = std::make_shared<Pattern>(pattern_str);
        PatternGenerator mem_gen(mem, pattern);
        PatternGenerator bin_gen(bin, pattern);
        for (std::uint32_t seed = 1; seed <= 8; ++seed) {
            for (std::size_t seq = 0; seq < 64; ++seq) {
                EXPECT_EQ(mem_gen(seed, seq), bin_gen(seed, seq))
                    << "pattern '" << pattern_str << "' seed=" << seed << " seq=" << seq;
            }
        }
    }
}

}  // namespace

UTEST(BinaryParity, InMemorySlugParity) {
    TestSlugParity(test::kDictionaryTestData, nullptr);
}

UTEST(BinaryParity, FileSlugParity) {
    auto file = std::make_shared<utils::MemoryMappedFile>(test::kTestDictionaryFile);
    TestSlugParity(file->data(), file);
}

}  // namespace slugkit::generator
