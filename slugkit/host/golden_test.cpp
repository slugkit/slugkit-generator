// Phase 0 golden regression test for the userver-free generator core.
//
// Ports the deterministic, committed expectations from slugkit/tests/generator_test.cpp into plain
// GoogleTest so we can prove the standalone (compat-shim + utf8proc) build produces byte-identical
// / property-stable slug output without userver. The mixed-case section mirrors the property-based
// form introduced by PR #20.

#include <slugkit/generator/generator.hpp>
#include <slugkit/generator/pattern_generator.hpp>
#include <slugkit/utils/text.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace slugkit::generator {

using namespace literals;

namespace {

const std::vector<Word> kNouns = {
    {"noun1", {}},
    {"noun2", {}},
    {"noun3", {"tag1"_tag}},
    {"noun4", {"tag2"_tag, "nsfw"_tag}},
    {"noun5", {"tag1"_tag, "tag2"_tag}},
};

const std::vector<Word> kAdjectives = {
    {"adjective1", {}},
    {"adjective2", {}},
    {"adjective3", {"tag1"_tag}},
    {"adjective4", {"tag2"_tag, "nsfw"_tag}},
    {"adjective5", {"tag1"_tag, "tag2"_tag}},
    {"adjective6", {"tag1"_tag, "tag2"_tag, "nsfw"_tag}},
    {"adjective7", {"tag1"_tag, "tag2"_tag, "nsfw"_tag}},
};

const std::vector<Word> kVerbs = {
    {"verb1", {}},
    {"verb2", {}},
    {"verb3", {"tag1"_tag}},
    {"verb4", {"tag2"_tag, "nsfw"_tag}},
    {"verb5", {"tag1"_tag, "tag2"_tag}},
    {"verb6", {"tag1"_tag, "tag2"_tag, "nsfw"_tag}},
    {"verb7", {"tag1"_tag, "tag2"_tag, "nsfw"_tag}},
    {"verb8", {"tag1"_tag, "tag2"_tag, "nsfw"_tag}},
    {"verb9", {"tag1"_tag, "tag2"_tag, "nsfw"_tag}},
    {"verb10", {"tag1"_tag, "tag2"_tag, "nsfw"_tag}},
};

const std::vector<Word> kAdverbs = {
    {"adverb1", {}},
    {"adverb2", {}},
    {"adverb3", {"tag1"_tag}},
    {"adverb4", {"tag2"_tag, "nsfw"_tag}},
    {"adverb5", {"tag1"_tag, "tag2"_tag}},
    {"adverb6", {"tag1"_tag, "tag2"_tag, "nsfw"_tag}},
    {"adverb7", {"tag1"_tag, "tag2"_tag, "nsfw"_tag}},
    {"adverb8", {"tag1"_tag, "tag2"_tag, "nsfw"_tag}},
    {"adverb9", {"tag1"_tag, "tag2"_tag, "nsfw"_tag}},
};

const std::vector<Word> kLanguageAgnosticNouns = {
    {"noun1", {}},
    {"noun2", {}},
    {"noun3", {"tag1"_tag}},
    {"noun4", {"tag2"_tag, "nsfw"_tag}},
    {"noun5", {"tag1"_tag, "tag2"_tag}},
};

DictionarySet MakeDictionarySet() {
    return DictionarySet{
        {
            Dictionary("noun", "en"_lang_view, kNouns),
            Dictionary("adjective", "en"_lang_view, kAdjectives),
            Dictionary("verb", "en"_lang_view, kVerbs),
            Dictionary("adverb", "en"_lang_view, kAdverbs),
            Dictionary("noun", ""_lang_view, kLanguageAgnosticNouns),
        },
    };
}

const Selector kNounSelector = "noun"_selector;
constexpr auto kTestSeed = "foobar";

}  // namespace

TEST(SubstitutionGenerator, LowerCaseWords) {
    Dictionary dictionary("noun", "en"_lang_view, kNouns);
    auto filtered = dictionary.Filter(kNounSelector);
    SelectorSubstitutionGenerator generator{filtered, {5, 5}};
    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
    EXPECT_EQ(generator.Generate(seed_hash, 0), "noun2");
    EXPECT_EQ(generator.Generate(seed_hash, 1), "noun3");
    EXPECT_EQ(generator.Generate(seed_hash, 2), "noun4");
    EXPECT_EQ(generator.Generate(seed_hash, 3), "noun5");
    EXPECT_EQ(generator.Generate(seed_hash, 4), "noun1");
    EXPECT_EQ(generator.Generate(seed_hash, 5), "noun2");
    EXPECT_EQ(generator.Generate(seed_hash, 6), "noun3");
}

TEST(SubstitutionGenerator, UpperCaseWords) {
    Dictionary dictionary("noun", "en"_lang_view, kNouns);
    auto filtered = dictionary.Filter("NOUN"_selector);
    SelectorSubstitutionGenerator generator{filtered, {5, 5}};
    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
    EXPECT_EQ(generator.Generate(seed_hash, 0), "NOUN2");
    EXPECT_EQ(generator.Generate(seed_hash, 1), "NOUN3");
    EXPECT_EQ(generator.Generate(seed_hash, 2), "NOUN4");
    EXPECT_EQ(generator.Generate(seed_hash, 3), "NOUN5");
    EXPECT_EQ(generator.Generate(seed_hash, 4), "NOUN1");
    EXPECT_EQ(generator.Generate(seed_hash, 5), "NOUN2");
    EXPECT_EQ(generator.Generate(seed_hash, 6), "NOUN3");
}

TEST(SubstitutionGenerator, TitleCaseWords) {
    Dictionary dictionary("noun", "en"_lang_view, kNouns);
    auto filtered = dictionary.Filter("Noun"_selector);
    SelectorSubstitutionGenerator generator{filtered, {5, 5}};
    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
    EXPECT_EQ(generator.Generate(seed_hash, 0), "Noun2");
    EXPECT_EQ(generator.Generate(seed_hash, 1), "Noun3");
    EXPECT_EQ(generator.Generate(seed_hash, 2), "Noun4");
    EXPECT_EQ(generator.Generate(seed_hash, 3), "Noun5");
    EXPECT_EQ(generator.Generate(seed_hash, 4), "Noun1");
    EXPECT_EQ(generator.Generate(seed_hash, 5), "Noun2");
    EXPECT_EQ(generator.Generate(seed_hash, 6), "Noun3");
}

TEST(SubstitutionGenerator, MixedCaseWords) {
    Dictionary dictionary("noun", "en"_lang_view, kNouns);
    auto filtered = dictionary.Filter("nOun"_selector);
    // Each noun is "nounN": four letters plus a digit, so it has 2^4 = 16 distinct cased forms
    // (the digit has no case). Five nouns -> mixed-case capacity 5 * 16 = 80.
    constexpr std::int64_t kMixedCapacity = 5 * 16;
    EXPECT_EQ(filtered->MixedCapacity(), static_cast<std::uint64_t>(kMixedCapacity));

    SelectorSubstitutionGenerator generator{filtered, {5, kMixedCapacity}};
    EXPECT_EQ(generator.GetCapacity(), kMixedCapacity);

    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
    // Every sequence value in [0, capacity) must yield a distinct slug (collision-free) and each
    // must be a cased form of one of the nouns. ToLower exercises the utf8proc path.
    const std::set<std::string> kBaseWords{"noun1", "noun2", "noun3", "noun4", "noun5"};
    std::set<std::string> seen;
    for (std::int64_t i = 0; i < kMixedCapacity; ++i) {
        auto value = generator.Generate(seed_hash, i);
        EXPECT_TRUE(kBaseWords.count(utils::text::ToLower(value, utils::text::kEnUsLocale)) == 1) << value;
        seen.insert(value);
    }
    EXPECT_EQ(seen.size(), static_cast<std::size_t>(kMixedCapacity));
}

TEST(SubstitutionGenerator, Numbers) {
    NumberSubstitutionGenerator generator{"number:2d"_number_gen};
    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
    EXPECT_EQ(generator.Generate(seed_hash, 0), "21");
    EXPECT_EQ(generator.Generate(seed_hash, 1), "42");
    EXPECT_EQ(generator.Generate(seed_hash, 2), "63");
    EXPECT_EQ(generator.Generate(seed_hash, 3), "84");
    EXPECT_EQ(generator.Generate(seed_hash, 4), "05");
    // rotation period is 100
    EXPECT_EQ(generator.Generate(seed_hash, 100), "21");
}

TEST(PatternGenerator, GetCapacity) {
    auto dictionaries = MakeDictionarySet();
    EXPECT_EQ(PatternGenerator(dictionaries, "{noun}"_pattern_ptr).GetCapacity(), 5);
    EXPECT_EQ(PatternGenerator(dictionaries, "{nOun}"_pattern_ptr).GetCapacity(), 80);
    EXPECT_EQ(PatternGenerator(dictionaries, "{adjective}-{noun}"_pattern_ptr).GetCapacity(), 35);
}

// Placeholder alternation: {a}|{b} chooses one child. Capacity is the sum of the children's
// capacities, it is deterministic, and collision-free over that capacity.
TEST(PatternGenerator, Alternation) {
    auto dictionaries = MakeDictionarySet();
    // noun=5, verb=10, adjective=7.
    EXPECT_EQ(PatternGenerator(dictionaries, "{noun}|{verb}"_pattern_ptr).GetCapacity(), 5 + 10);
    EXPECT_EQ(PatternGenerator(dictionaries, "{noun}|{adjective}"_pattern_ptr).GetCapacity(), 5 + 7);
    EXPECT_EQ(PatternGenerator(dictionaries, "{noun}|{verb}|{adjective}"_pattern_ptr).GetCapacity(), 5 + 10 + 7);
    // Surrounding text keeps a single alternation placeholder.
    EXPECT_EQ(PatternGenerator(dictionaries, "x-{noun}|{verb}-y"_pattern_ptr).GetCapacity(), 15);

    PatternGenerator generator(dictionaries, "{noun}|{verb}"_pattern_ptr);
    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
    std::set<std::string> nouns{"noun1", "noun2", "noun3", "noun4", "noun5"};
    std::set<std::string> verbs;
    for (int i = 1; i <= 10; ++i) {
        verbs.insert("verb" + std::to_string(i));
    }
    const auto capacity = static_cast<std::uint64_t>(generator.GetCapacity());
    ASSERT_EQ(capacity, 15u);
    std::set<std::string> seen;
    for (std::uint64_t i = 0; i < capacity; ++i) {
        auto slug = generator(seed_hash, i);
        EXPECT_TRUE(nouns.count(slug) == 1 || verbs.count(slug) == 1) << slug;
        seen.insert(slug);
    }
    EXPECT_EQ(seen.size(), capacity);  // collision-free: all 15 distinct
}

// End-to-end generator: committed full-slug expectations from generator_test.cpp (GenerateID).
TEST(Generator, GenerateID) {
    Generator generator(MakeDictionarySet());
    auto pattern = "-{adjective}-{adverb}-{noun}-{number:2d}-"_pattern_ptr;
    auto settings = generator.GetCapacity(pattern);

    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 0), "-adjective6-adverb8-noun1-53-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 1), "-adjective3-adverb6-noun5-06-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 2), "-adjective7-adverb4-noun4-59-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 3), "-adjective4-adverb2-noun3-12-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 4), "-adjective1-adverb9-noun2-65-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 5), "-adjective5-adverb7-noun1-18-");
}

// Self-consistency: the in-memory generator must be stable across calls for a simple pattern.
TEST(Generator, SelfConsistent) {
    Generator generator(MakeDictionarySet());
    const char* pattern = "{adjective}-{noun}";
    for (std::size_t n : {std::size_t{0}, std::size_t{1}, std::size_t{7}, std::size_t{34}}) {
        auto a = generator.Generate(pattern, kTestSeed, n);
        auto b = generator.Generate(pattern, kTestSeed, n);
        EXPECT_EQ(a, b) << "n=" << n;
        EXPECT_FALSE(a.empty());
    }
}

// Turkish probe: captures the utf8proc casing reference (no ICU parity assumed). Asserts only
// determinism, and prints the produced bytes for the report.
TEST(TextCasing, TurkishProbe) {
    // UTF-8 bytes for "İSTANBUL gıda Ğit" (İ=C4 B0, ı=C4 B1, Ğ=C4 9E). Segments are concatenated so
    // a \x escape never greedily absorbs a following hex-digit letter (e.g. "da" after \xB1).
    const std::string turkish = "\xC4\xB0" "STANBUL g" "\xC4\xB1" "da " "\xC4\x9E" "it";
    auto lower1 = utils::text::ToLower(turkish, utils::text::kEnUsLocale);
    auto lower2 = utils::text::ToLower(turkish, utils::text::kEnUsLocale);
    auto upper1 = utils::text::ToUpper(turkish, utils::text::kEnUsLocale);
    auto upper2 = utils::text::ToUpper(turkish, utils::text::kEnUsLocale);
    EXPECT_EQ(lower1, lower2);
    EXPECT_EQ(upper1, upper2);
    RecordProperty("turkish_input", turkish);
    RecordProperty("turkish_lower", lower1);
    RecordProperty("turkish_upper", upper1);
    std::cerr << "[Turkish probe] input : " << turkish << "\n"
              << "[Turkish probe] lower : " << lower1 << "\n"
              << "[Turkish probe] upper : " << upper1 << std::endl;
}

}  // namespace slugkit::generator
