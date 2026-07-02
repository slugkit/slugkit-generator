// Phase 0 golden regression test for the userver-free generator core.
//
// Ports the deterministic, committed expectations from slugkit/tests/generator_test.cpp into plain
// GoogleTest so we can prove the standalone (compat-shim + utf8proc) build produces byte-identical
// / property-stable slug output without userver. The mixed-case section mirrors the property-based
// form introduced by PR #20.

#include <slugkit/generator/binary_dictionary.hpp>
#include <slugkit/generator/exceptions.hpp>
#include <slugkit/generator/generator.hpp>
#include <slugkit/generator/pattern_generator.hpp>
#include <slugkit/utils/memory_mapped_file.hpp>
#include <slugkit/utils/text.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <memory>
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
    // Golden: the exact deterministic output over the full cycle (seed "foobar"). Also collision-free
    // (all 15 distinct: five nouns interleaved with ten verbs).
    const std::vector<std::string> kExpected = {
        "verb7", "noun1", "verb10", "noun4", "verb3",  "noun2", "verb6", "verb5",
        "verb9", "verb8", "verb2",  "verb1", "noun5",  "verb4", "noun3",
    };
    ASSERT_EQ(kExpected.size(), 15u);
    std::set<std::string> seen;
    for (std::uint64_t i = 0; i < kExpected.size(); ++i) {
        auto slug = generator(seed_hash, i);
        EXPECT_EQ(slug, kExpected[i]) << "seq " << i;
        seen.insert(slug);
    }
    EXPECT_EQ(seen.size(), kExpected.size());  // collision-free
}

// Equivalent alternatives (same predicate) collapse to one; alternatives whose outputs actually
// intersect are a pattern error.
TEST(PatternGenerator, AlternationCollapseAndDisjoint) {
    auto dictionaries = MakeDictionarySet();
    // {noun}|{noun} collapses -> capacity is 5, not 10 (no double counting, no collisions).
    EXPECT_EQ(PatternGenerator(dictionaries, "{noun}|{noun}"_pattern_ptr).GetCapacity(), 5);
    EXPECT_EQ(PatternGenerator(dictionaries, "{noun}|{noun}|{adjective}"_pattern_ptr).GetCapacity(), 12);
    // Collapse is type-agnostic: identical number/special generators collapse too.
    EXPECT_EQ(PatternGenerator(dictionaries, "{number:2d}|{number:2d}"_pattern_ptr).GetCapacity(), 100);
    // Distinct-but-disjoint number generators are kept ("00".."99" vs "000".."999" never coincide).
    EXPECT_EQ(PatternGenerator(dictionaries, "{number:2d}|{number:3d}"_pattern_ptr).GetCapacity(), 1100);
    // But decimal and hex 2-digit overlap (e.g. "27") -> error.
    EXPECT_THROW(PatternGenerator(dictionaries, "{number:2d}|{number:2x}"_pattern_ptr), PatternSyntaxError);
    // {noun} is a superset of {noun:+tag1}, so their outputs overlap -> error.
    EXPECT_THROW(PatternGenerator(dictionaries, "{noun:+tag1}|{noun}"_pattern_ptr), PatternSyntaxError);
    // Disjoint dictionaries (noun* vs verb*) are fine.
    EXPECT_NO_THROW(PatternGenerator(dictionaries, "{noun}|{verb}"_pattern_ptr));
}

// Group alternation: parenthesised branches lock correlated choices; lone/collapsed groups pull up.
TEST(PatternGenerator, GroupAlternation) {
    auto dictionaries = MakeDictionarySet();
    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);

    // Pull-up: a lone group is transparent -- same capacity AND same output as unparenthesised.
    EXPECT_EQ(PatternGenerator(dictionaries, "({noun})"_pattern_ptr).GetCapacity(), 5);
    EXPECT_EQ(PatternGenerator(dictionaries, "({adjective} {noun})"_pattern_ptr).GetCapacity(), 35);
    EXPECT_EQ(PatternGenerator(dictionaries, "pre-({adjective} {noun})-post"_pattern_ptr).GetCapacity(), 35);
    {
        PatternGenerator grouped(dictionaries, "({adjective} {noun})"_pattern_ptr);
        PatternGenerator plain(dictionaries, "{adjective} {noun}"_pattern_ptr);
        for (std::uint64_t i = 0; i < 20; ++i) {
            EXPECT_EQ(grouped(seed_hash, i), plain(seed_hash, i)) << "i=" << i;  // pull-up is exact
        }
    }

    // Group alternation capacity is the sum of the branches' LCMs.
    // LCM(noun=5, verb=10)=10; LCM(adjective=7, adverb=9)=63; total 73.
    EXPECT_EQ(
        PatternGenerator(dictionaries, "({noun} {verb})|({adjective} {adverb})"_pattern_ptr).GetCapacity(), 73
    );
    // Text-only branches.
    EXPECT_EQ(PatternGenerator(dictionaries, "(foo)|(bar)"_pattern_ptr).GetCapacity(), 2);

    // Disjoint branches are fine (noun*/verb* vs adjective*/adverb* never coincide).
    EXPECT_NO_THROW(PatternGenerator(dictionaries, "({noun} {verb})|({adjective} {adverb})"_pattern_ptr));
    // Overlap is a pattern error: at every aligned position the branches can coincide
    // ({noun:+tag1} is a subset of {noun}; the second placeholder is identical).
    EXPECT_THROW(
        PatternGenerator(dictionaries, "({noun:+tag1} {adjective})|({noun} {adjective})"_pattern_ptr),
        PatternSyntaxError
    );

    // An empty branch `()` is an explicit "or nothing" option (capacity +1) among other branches.
    {
        PatternGenerator optional_noun(dictionaries, "({noun})|()"_pattern_ptr);
        EXPECT_EQ(optional_noun.GetCapacity(), 6);  // 5 nouns + the empty option
        std::set<std::string> seen;
        for (std::uint64_t i = 0; i < 6; ++i) {
            seen.insert(optional_noun(seed_hash, i));
        }
        EXPECT_EQ(seen.count(""), 1u);  // the empty string is produced
        EXPECT_EQ(seen.size(), 6u);     // collision-free: 5 nouns + ""
    }
    // A standalone or all-empty alternation is rejected.
    EXPECT_THROW(ParsePattern("()"), PatternSyntaxError);
    EXPECT_THROW(ParsePattern("()|()"), PatternSyntaxError);
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

#ifdef SLK_ADVERB_BIN_PATH
namespace {
// Load a compiled binary dictionary into a set. The memory-mapped file is the keepalive: it owns
// the bytes and is retained by the set, so the mapping outlives the dictionaries that view it.
binary::DictionarySet LoadBinaryDictionary(const char* path) {
    auto mapping = std::make_shared<utils::MemoryMappedFile>(path);
    binary::DictionarySet dictionaries;
    dictionaries.Add(mapping->data(), mapping);
    return dictionaries;
}
}  // namespace

// Honest opt-ins (binary path): test-adv marks `nsfw` opt-in (4 adverbs). They are hidden by
// default, selectable with an explicit `+nsfw`, and unhidden by the per-generator usage flag.
TEST(Generator, OptInTags) {
    Generator generator(LoadBinaryDictionary(SLK_ADVERB_BIN_PATH));

    // Default: the 4 nsfw adverbs are hidden -> pool is 3619 - 4 = 3615.
    EXPECT_EQ(generator.GetCapacity("{adverb}").capacity, 3615);
    // Explicit request still selects exactly the opt-in words.
    EXPECT_EQ(generator.GetCapacity("{adverb:+nsfw}").capacity, 4);

    // Enabling the tag lifts its gate: the pool returns to the full 3619 and the output matches
    // what the engine produced before opt-in filtering existed.
    generator.EnableOptIn("nsfw");
    EXPECT_EQ(generator.GetCapacity("{adverb}").capacity, 3619);
    EXPECT_EQ(generator.Generate("{adverb}", "foobar", 0), "lustfully");
    EXPECT_EQ(generator.EnabledOptIns(), (std::vector<std::string>{"nsfw"}));

    // Clearing restores the default hidden behaviour. Reusing the same generator (whose shared
    // filter cache already holds the enabled=3619 entry) proves the cache key accounts for the
    // opt-in state: a stale entry would wrongly return 3619 here.
    generator.ClearOptIns();
    EXPECT_EQ(generator.GetCapacity("{adverb}").capacity, 3615);

    // Enabling a tag this dictionary does not have is inert: the adverb pool stays 3615 (and its
    // cache key is unchanged), since only a dictionary's own opt-in tags affect its result.
    generator.EnableOptIn("no-such-opt-in-tag");
    EXPECT_EQ(generator.GetCapacity("{adverb}").capacity, 3615);
}
#endif  // SLK_ADVERB_BIN_PATH

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
