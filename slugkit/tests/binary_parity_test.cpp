// Generation parity: a PatternGenerator fed a binary (memory-mapped) dictionary set must
// produce byte-identical slugs to one fed the equivalent in-memory dictionary set. The
// binary set is decompiled into an in-memory set so both carry identical data, then the two
// generators are compared across selector shapes, cases, seeds and sequence numbers.
#include <slugkit/generator/binary_dictionary.hpp>
#include <slugkit/generator/dictionary.hpp>
#include <slugkit/generator/generator.hpp>
#include <slugkit/generator/pattern.hpp>
#include <slugkit/generator/pattern_generator.hpp>

#include <slugkit/test_utils/test_dictionary.hpp>
#include <slugkit/test_utils/data_config.hpp>

#include <slugkit/utils/memory_mapped_file.hpp>

#include <userver/utest/utest.hpp>

#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <vector>

namespace slugkit::generator {

using namespace literals;

namespace {

// A small language-agnostic dictionary (empty language code), like domain/shell. Compiled
// from a YAML with no `tags:` section AND a header size that is a multiple of 4, so it
// exercises the agnostic, tagless, and section-alignment-padding paths at once.
alignas(4) constexpr unsigned char kGizmoData[] = {
  0x53, 0x4c, 0x55, 0x47, 0x44, 0x49, 0x43, 0x54, 0x02, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x05, 0x00, 0x0a, 0x00, 0x16, 0x00,
  0x67, 0x69, 0x7a, 0x6d, 0x6f, 0x31, 0x2e, 0x30, 0x2e, 0x30, 0x61, 0x67,
  0x6e, 0x6f, 0x73, 0x74, 0x69, 0x63, 0x20, 0x70, 0x61, 0x64, 0x64, 0x69,
  0x6e, 0x67, 0x20, 0x74, 0x65, 0x73, 0x74, 0x21, 0x49, 0x4e, 0x44, 0x45,
  0x58, 0x3d, 0x3d, 0x3d, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x1b, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00,
  0x72, 0x00, 0x00, 0x00, 0x8a, 0x00, 0x00, 0x00, 0x4c, 0x41, 0x4e, 0x47,
  0x2d, 0x54, 0x41, 0x42, 0x4c, 0x45, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
  0x6c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
  0x4c, 0x45, 0x4e, 0x47, 0x54, 0x48, 0x2d, 0x49, 0x4e, 0x44, 0x45, 0x58,
  0x3d, 0x3d, 0x3d, 0x3d, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x54, 0x41, 0x47, 0x53,
  0x2d, 0x54, 0x41, 0x42, 0x4c, 0x45, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
  0x6c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x24, 0x00, 0x00, 0x00, 0x54, 0x41, 0x47, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
  0x00, 0x00, 0x06, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x63, 0x6f, 0x6d,
  0x6d, 0x6f, 0x6e, 0x2a, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x54, 0x41, 0x47, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
  0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x74, 0x6f, 0x6f,
  0x6c, 0x2a, 0x2a, 0x2a, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x57, 0x4f, 0x52, 0x44,
  0x53, 0x3d, 0x3d, 0x3d, 0xbb, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x05, 0x00, 0x0a, 0x00, 0x05, 0x00,
  0x61, 0x6c, 0x70, 0x68, 0x61, 0x41, 0x4c, 0x50, 0x48, 0x41, 0x41, 0x6c,
  0x70, 0x68, 0x61, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x05, 0x00, 0x0a,
  0x00, 0x05, 0x00, 0x62, 0x72, 0x61, 0x76, 0x6f, 0x42, 0x52, 0x41, 0x56,
  0x4f, 0x42, 0x72, 0x61, 0x76, 0x6f, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00,
  0x07, 0x00, 0x0e, 0x00, 0x07, 0x00, 0x63, 0x68, 0x61, 0x72, 0x6c, 0x69,
  0x65, 0x43, 0x48, 0x41, 0x52, 0x4c, 0x49, 0x45, 0x43, 0x68, 0x61, 0x72,
  0x6c, 0x69, 0x65, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x05, 0x00, 0x0a,
  0x00, 0x05, 0x00, 0x64, 0x65, 0x6c, 0x74, 0x61, 0x44, 0x45, 0x4c, 0x54,
  0x41, 0x44, 0x65, 0x6c, 0x74, 0x61, 0x00, 0x00, 0x04, 0x00, 0x04, 0x00,
  0x04, 0x00, 0x08, 0x00, 0x04, 0x00, 0x65, 0x63, 0x68, 0x6f, 0x45, 0x43,
  0x48, 0x4f, 0x45, 0x63, 0x68, 0x6f, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00,
  0x07, 0x00, 0x0e, 0x00, 0x07, 0x00, 0x66, 0x6f, 0x78, 0x74, 0x72, 0x6f,
  0x74, 0x46, 0x4f, 0x58, 0x54, 0x52, 0x4f, 0x54, 0x46, 0x6f, 0x78, 0x74,
  0x72, 0x6f, 0x74
};
constexpr unsigned int kGizmoLen = 495;

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

// The top-level Generator (which dispatches over an in-memory / binary dictionary-set
// variant) must likewise produce identical slugs through its batch callback API.
void TestGeneratorParity(std::span<const std::byte> data, std::shared_ptr<void> keepalive) {
    binary::BinaryDictionary dict(data);
    Generator mem_gen(BuildInMemorySet(dict));

    binary::DictionarySet bin_set;
    bin_set.Add(data, std::move(keepalive));
    Generator bin_gen(std::move(bin_set));

    for (const auto& pattern_str : kPatterns) {
        std::vector<std::string> mem_out;
        std::vector<std::string> bin_out;
        mem_gen.Generate(pattern_str, "parity-seed", 0, 32, [&](std::string s) { mem_out.push_back(std::move(s)); });
        bin_gen.Generate(pattern_str, "parity-seed", 0, 32, [&](std::string s) { bin_out.push_back(std::move(s)); });
        EXPECT_EQ(mem_out, bin_out) << "pattern '" << pattern_str << "'";
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

UTEST(BinaryParity, GeneratorSlugParity) {
    TestGeneratorParity(test::kDictionaryTestData, nullptr);
}

// A language-agnostic dictionary (domain/shell style: empty language code) must resolve for a
// no-language selector instead of defaulting to "en", and stay byte-identical to the
// in-memory set built from the same data.
UTEST(BinaryParity, AgnosticLanguageResolution) {
    auto data = std::as_bytes(std::span{kGizmoData, static_cast<std::size_t>(kGizmoLen)});
    binary::DictionarySet bin;
    bin.Add(data, nullptr);

    auto all = bin.Filter("gizmo"_selector);
    ASSERT_TRUE(static_cast<bool>(all));
    EXPECT_EQ(all->size(), 6u);
    EXPECT_EQ(bin.Filter("gizmo:+tool"_selector)->size(), 3u);
    EXPECT_EQ(bin.Filter("gizmo:+common"_selector)->size(), 2u);
    // an explicit, absent language yields empty (matching the in-memory set)
    auto en = bin.Filter("gizmo@en"_selector);
    EXPECT_TRUE(!en || en->empty());

    binary::BinaryDictionary dict(data);
    auto mem = BuildInMemorySet(dict);
    for (const auto* pattern_str : {"{gizmo}", "{gizmo:+tool}", "{gizmo:+common}"}) {
        auto pattern = std::make_shared<Pattern>(std::string(pattern_str));
        PatternGenerator mem_gen(mem, pattern);
        PatternGenerator bin_gen(bin, pattern);
        for (std::uint32_t seed = 1; seed <= 4; ++seed) {
            for (std::size_t seq = 0; seq < 24; ++seq) {
                EXPECT_EQ(mem_gen(seed, seq), bin_gen(seed, seq)) << "pattern '" << pattern_str << "'";
            }
        }
    }
}

}  // namespace slugkit::generator
