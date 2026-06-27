// Emoji generation over the file-based dictionary path. Emoji is a regular dictionary kind now
// (compiled to emoji.bin, committed under tests/data and built from db/dicts/emoji.yaml), sourced
// from the binary or in-memory DictionarySet like any selector. These tests drive the real emoji
// dictionary through the binary path and check exact output sequences, capacities, and that the
// binary and in-memory paths agree (parity).
#include <slugkit/generator/binary_dictionary.hpp>
#include <slugkit/generator/dictionary.hpp>
#include <slugkit/generator/pattern.hpp>
#include <slugkit/generator/pattern_generator.hpp>

#include <slugkit/test_utils/test_dictionary.hpp>

#include <userver/utest/utest.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace slugkit::generator {

using namespace literals;

namespace {

binary::DictionarySet EmojiSet() {
    binary::DictionarySet bin;
    bin.Add(test::kEmojiTestData, nullptr);
    return bin;
}

// Build an emoji generator over the real binary dictionary, filtering by `selector`'s tags and
// applying `emoji_gen`'s count/unique options -- the same wiring CalculateSettings performs.
BinaryEmojiSubstitutionGenerator MakeEmoji(const binary::DictionarySet& bin, const Selector& selector, const EmojiGen& emoji_gen) {
    return BinaryEmojiSubstitutionGenerator{bin.Filter(selector), emoji_gen};
}

// Decompile a BinaryDictionary into an in-memory DictionarySet carrying identical words and tags,
// so the two generation paths can be compared on the same data.
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

}  // namespace

UTEST(EmojiGenerator, BinaryDictionaryLoads) {
    binary::BinaryDictionary dict(test::kEmojiTestData);
    EXPECT_EQ(dict.Kind(), "emoji");
    EXPECT_GT(dict.Count().GetUnderlying(), 0u);

    auto bin = EmojiSet();
    EXPECT_GT(bin.Filter("emoji"_selector)->size(), 0u);
    EXPECT_GT(bin.Filter("emoji:+face"_selector)->size(), 0u);
}

namespace {

// n^k: number of length-k sequences with repetition.
std::uint64_t Pow(std::uint64_t n, std::uint64_t k) {
    std::uint64_t r = 1;
    for (std::uint64_t i = 0; i < k; ++i) {
        r *= n;
    }
    return r;
}

// n*(n-1)*...*(n-k+1): number of length-k sequences without repetition.
std::uint64_t Falling(std::uint64_t n, std::uint64_t k) {
    std::uint64_t r = 1;
    for (std::uint64_t i = 0; i < k; ++i) {
        r *= (n - i);
    }
    return r;
}

}  // namespace

// Capacities are derived from the real filtered emoji counts rather than magic numbers, so the
// tests still pin the permutation maths but survive benign edits to emoji.yaml. Exact byte-level
// output is covered by BinaryParity (in-memory vs binary) below.
UTEST(EmojiGenerator, Generate) {
    auto bin = EmojiSet();
    const auto total = bin.Filter("emoji"_selector)->size();
    auto generator = MakeEmoji(bin, "emoji"_selector, "emoji"_emoji_gen);
    EXPECT_EQ(generator.GetMaxLength(), 1);
    EXPECT_EQ(generator.GetCapacity(), total);
    auto seed_hash = PatternGenerator::SeedHash("test");
    for (std::size_t seq = 0; seq < 8; ++seq) {
        auto value = generator.Generate(seed_hash, seq);
        EXPECT_FALSE(value.empty());
        EXPECT_EQ(value, generator.Generate(seed_hash, seq));  // deterministic
    }
}

UTEST(EmojiGenerator, GenerateFaces) {
    auto bin = EmojiSet();
    const std::uint64_t faces = bin.Filter("emoji:+face"_selector)->size();
    auto generator = MakeEmoji(bin, "emoji:+face"_selector, "emoji:+face"_emoji_gen);
    EXPECT_EQ(generator.GetMaxLength(), 1);
    EXPECT_EQ(generator.GetCapacity(), faces);
    auto seed_hash = PatternGenerator::SeedHash("test");
    for (std::size_t seq = 0; seq < 8; ++seq) {
        EXPECT_FALSE(generator.Generate(seed_hash, seq).empty());
    }
}

UTEST(EmojiGenerator, GenerateFaceSetNonUnique) {
    auto bin = EmojiSet();
    const std::uint64_t faces = bin.Filter("emoji:+face"_selector)->size();
    auto seed_hash = PatternGenerator::SeedHash("test");
    {
        auto generator = MakeEmoji(bin, "emoji:+face"_selector, "emoji:+face count=2"_emoji_gen);
        EXPECT_EQ(generator.GetMaxLength(), 2);
        EXPECT_EQ(generator.GetCapacity(), Pow(faces, 2));
    }
    {
        auto generator = MakeEmoji(bin, "emoji:+face"_selector, "emoji:+face count=6"_emoji_gen);
        EXPECT_EQ(generator.GetMaxLength(), 6);
        EXPECT_EQ(generator.GetCapacity(), Pow(faces, 6));
    }
    {
        auto generator = MakeEmoji(bin, "emoji:+face"_selector, "emoji:+face count=0-6"_emoji_gen);
        EXPECT_EQ(generator.GetMaxLength(), 6);
        std::uint64_t expected = 0;
        for (std::uint64_t k = 0; k <= 6; ++k) {
            expected += Pow(faces, k);
        }
        EXPECT_EQ(generator.GetCapacity(), expected);
    }
}

UTEST(EmojiGenerator, GenerateFaceSetUnique) {
    auto bin = EmojiSet();
    const std::uint64_t faces = bin.Filter("emoji:+face"_selector)->size();
    {
        auto generator = MakeEmoji(bin, "emoji:+face"_selector, "emoji:+face count=2 unique=true"_emoji_gen);
        EXPECT_EQ(generator.GetMaxLength(), 2);
        EXPECT_EQ(generator.GetCapacity(), Falling(faces, 2));
    }
    {
        auto generator = MakeEmoji(bin, "emoji:+face"_selector, "emoji:+face count=6 unique=true"_emoji_gen);
        EXPECT_EQ(generator.GetMaxLength(), 6);
        EXPECT_EQ(generator.GetCapacity(), Falling(faces, 6));
    }
    {
        auto generator = MakeEmoji(bin, "emoji:+face"_selector, "emoji:+face count=0-6 unique=true"_emoji_gen);
        EXPECT_EQ(generator.GetMaxLength(), 6);
        std::uint64_t expected = 0;
        for (std::uint64_t k = 0; k <= 6; ++k) {
            expected += Falling(faces, k);
        }
        EXPECT_EQ(generator.GetCapacity(), expected);
    }
}

// The in-memory and binary paths must produce byte-identical emoji slugs across counts, tags, the
// unique flag, seeds and sequence numbers -- this covers the new BinaryEmojiSubstitutionGenerator
// against the in-memory EmojiSubstitutionGenerator on identical data.
UTEST(EmojiGenerator, BinaryParity) {
    binary::BinaryDictionary dict(test::kEmojiTestData);
    auto mem = BuildInMemorySet(dict);
    auto bin = EmojiSet();

    const std::vector<std::string> patterns = {
        "{emoji}",
        "{emoji:+face}",
        "{emoji:+face count=2}",
        "{emoji:+face count=3 unique=true}",
        "{emoji:+face count=0-3}",
        "{emoji:-face count=2}",
    };
    for (const auto& pattern_str : patterns) {
        auto pattern = std::make_shared<Pattern>(pattern_str);
        PatternGenerator mem_gen(mem, pattern);
        PatternGenerator bin_gen(bin, pattern);
        EXPECT_EQ(mem_gen.GetCapacity(), bin_gen.GetCapacity()) << "pattern '" << pattern_str << "'";
        for (std::uint32_t seed = 1; seed <= 6; ++seed) {
            for (std::size_t seq = 0; seq < 48; ++seq) {
                EXPECT_EQ(mem_gen(seed, seq), bin_gen(seed, seq))
                    << "pattern '" << pattern_str << "' seed=" << seed << " seq=" << seq;
            }
        }
    }
}

}  // namespace slugkit::generator
