#include <slugkit/generator/binary_dictionary.hpp>
#include <slugkit/generator/exceptions.hpp>
#include <slugkit/generator/generator.hpp>
#include <slugkit/generator/pattern_generator.hpp>
#include <slugkit/utils/text.hpp>

#include <slugkit/test_utils/test_dictionary.hpp>

#include <userver/utest/utest.hpp>

#include <iostream>
#include <set>

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

const std::map<std::string, Dictionary> kDictionaries = {
    {"noun", Dictionary("noun", "en"_lang_view, kNouns)},
    {"adjective", Dictionary("adjective", "en"_lang_view, kAdjectives)},
    {"verb", Dictionary("verb", "en"_lang_view, kVerbs)},
    {"adverb", Dictionary("adverb", "en"_lang_view, kAdverbs)},
};

const DictionarySet kDictionariesSet{
    {
        Dictionary("noun", "en"_lang_view, kNouns),
        Dictionary("adjective", "en"_lang_view, kAdjectives),
        Dictionary("verb", "en"_lang_view, kVerbs),
        Dictionary("adverb", "en"_lang_view, kAdverbs),
        Dictionary("noun", ""_lang_view, kLanguageAgnosticNouns),
    },
};

const Selector kNounSelector = "noun"_selector;
const Selector kEnNounSelector = "noun@en"_selector;
constexpr auto kTestSeed = "foobar";

}  // namespace

UTEST(SubstitutionGenerator, LowerCaseWords) {
    Dictionary dictionary("noun", "en"_lang_view, kNouns);
    auto filtered_dictionary = dictionary.Filter(kNounSelector);
    SelectorSubstitutionGenerator generator{filtered_dictionary, {5, 5}};
    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
    EXPECT_EQ(generator.Generate(seed_hash, 0), "noun2");
    EXPECT_EQ(generator.Generate(seed_hash, 1), "noun3");
    EXPECT_EQ(generator.Generate(seed_hash, 2), "noun4");
    EXPECT_EQ(generator.Generate(seed_hash, 3), "noun5");
    EXPECT_EQ(generator.Generate(seed_hash, 4), "noun1");
    EXPECT_EQ(generator.Generate(seed_hash, 5), "noun2");
    EXPECT_EQ(generator.Generate(seed_hash, 6), "noun3");
}

UTEST(SubstitutionGenerator, UpperCaseWords) {
    Dictionary dictionary("noun", "en"_lang_view, kNouns);

    auto filtered_dictionary = dictionary.Filter("NOUN"_selector);
    SelectorSubstitutionGenerator generator{filtered_dictionary, {5, 5}};
    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
    EXPECT_EQ(generator.Generate(seed_hash, 0), "NOUN2");
    EXPECT_EQ(generator.Generate(seed_hash, 1), "NOUN3");
    EXPECT_EQ(generator.Generate(seed_hash, 2), "NOUN4");
    EXPECT_EQ(generator.Generate(seed_hash, 3), "NOUN5");
    EXPECT_EQ(generator.Generate(seed_hash, 4), "NOUN1");
    EXPECT_EQ(generator.Generate(seed_hash, 5), "NOUN2");
    EXPECT_EQ(generator.Generate(seed_hash, 6), "NOUN3");
}

UTEST(SubstitutionGenerator, TitleCaseWords) {
    Dictionary dictionary("noun", "en"_lang_view, kNouns);

    auto filtered_dictionary = dictionary.Filter("Noun"_selector);
    SelectorSubstitutionGenerator generator{filtered_dictionary, {5, 5}};
    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
    EXPECT_EQ(generator.Generate(seed_hash, 0), "Noun2");
    EXPECT_EQ(generator.Generate(seed_hash, 1), "Noun3");
    EXPECT_EQ(generator.Generate(seed_hash, 2), "Noun4");
    EXPECT_EQ(generator.Generate(seed_hash, 3), "Noun5");
    EXPECT_EQ(generator.Generate(seed_hash, 4), "Noun1");
    EXPECT_EQ(generator.Generate(seed_hash, 5), "Noun2");
    EXPECT_EQ(generator.Generate(seed_hash, 6), "Noun3");
}

UTEST(SubstitutionGenerator, MixedCaseWords) {
    Dictionary dictionary("noun", "en"_lang_view, kNouns);

    auto filtered_dictionary = dictionary.Filter("nOun"_selector);
    // Each noun is "nounN": four letters plus a digit, so it has 2^4 = 16 distinct cased forms
    // (the digit has no case). Five nouns -> mixed-case capacity 5 * 16 = 80.
    constexpr std::int64_t kMixedCapacity = 5 * 16;
    EXPECT_EQ(filtered_dictionary->MixedCapacity(), static_cast<std::uint64_t>(kMixedCapacity));

    SelectorSubstitutionGenerator generator{filtered_dictionary, {5, kMixedCapacity}};
    EXPECT_EQ(generator.GetCapacity(), kMixedCapacity);

    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
    // Every sequence value in [0, capacity) must yield a distinct slug (collision-free) and each
    // must be a cased form of one of the nouns.
    const std::set<std::string> kBaseWords{"noun1", "noun2", "noun3", "noun4", "noun5"};
    std::set<std::string> seen;
    for (std::int64_t i = 0; i < kMixedCapacity; ++i) {
        auto value = generator.Generate(seed_hash, i);
        EXPECT_TRUE(kBaseWords.count(utils::text::ToLower(value, utils::text::kEnUsLocale)) == 1) << value;
        seen.insert(value);
    }
    EXPECT_EQ(seen.size(), static_cast<std::size_t>(kMixedCapacity));
}

UTEST(SubstitutionGenerator, Numbers) {
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

UTEST(SubstitutionGenerator, HexNumbers) {
    {
        NumberSubstitutionGenerator generator{"number:2x"_number_gen};
        auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
        EXPECT_EQ(generator.Generate(seed_hash, 0), "9d");
        EXPECT_EQ(generator.Generate(seed_hash, 1), "01");
        EXPECT_EQ(generator.Generate(seed_hash, 2), "b1");
        EXPECT_EQ(generator.Generate(seed_hash, 3), "03");
        EXPECT_EQ(generator.Generate(seed_hash, 4), "06");
    }
    {
        NumberSubstitutionGenerator generator{"number:16x"_number_gen};
        auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
        EXPECT_EQ(generator.Generate(seed_hash, 0), "c969bc6ba7ad9a97");
        EXPECT_EQ(generator.Generate(seed_hash, 1), "46be3ac990fc2c98");
        EXPECT_EQ(generator.Generate(seed_hash, 2), "4ab47b0f83890218");
        EXPECT_EQ(generator.Generate(seed_hash, 3), "ec422f95ad0e9d00");
        EXPECT_EQ(generator.Generate(seed_hash, 4), "d98a675c7b068bbf");
    }
}

UTEST(SubstitutionGenerator, Roman) {
    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
    {
        RomanSubstitutionGenerator generator{"number:2R"_number_gen};
        EXPECT_EQ(generator.Generate(seed_hash, 0), "CI");
        EXPECT_EQ(generator.Generate(seed_hash, 1), "ML");
        EXPECT_EQ(generator.Generate(seed_hash, 2), "LV");
        EXPECT_EQ(generator.Generate(seed_hash, 3), "M");
        EXPECT_EQ(generator.Generate(seed_hash, 4), "XX");

        for (std::uint64_t i = 0; i < generator.GetCapacity(); ++i) {
            auto value = generator.Generate(seed_hash, i);
            EXPECT_LE(value.size(), 2);
        }
    }

    {
        RomanSubstitutionGenerator upper_gen{"number:15R"_number_gen};
        RomanSubstitutionGenerator lower_gen{"number:15r"_number_gen};
        std::set<std::string> values;
        for (std::uint64_t i = 0; i < 3999; ++i) {
            values.insert(upper_gen.Generate(seed_hash, i));
            values.insert(lower_gen.Generate(seed_hash, i));
        }
        EXPECT_EQ(values.size(), 3999 * 2);
        for (std::uint64_t i = 0; i < 3999; ++i) {
            EXPECT_EQ(
                utils::text::ToLower(upper_gen.Generate(seed_hash, i), utils::text::kEnUsLocale),
                lower_gen.Generate(seed_hash, i)
            );
        }
    }
}

UTEST(SubstitutionGenerator, Special) {
    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
    SpecialSubstitutionGenerator generator{"special:3"_special_gen};
    EXPECT_EQ(generator.Generate(seed_hash, 0), ")'#");
    EXPECT_EQ(generator.Generate(seed_hash, 1), "@|_");
    EXPECT_EQ(generator.Generate(seed_hash, 2), "[|*");
    EXPECT_EQ(generator.Generate(seed_hash, 3), "\"'$");
    EXPECT_EQ(generator.Generate(seed_hash, 4), "<|-");

    for (std::uint64_t i = 0; i < generator.GetCapacity(); ++i) {
        auto value = generator.Generate(seed_hash, i);
        EXPECT_EQ(value.size(), 3);
    }
}

UTEST(SubstitutionGenerator, SpecialCapacity) {
    {
        SpecialSubstitutionGenerator generator{"special:1"_special_gen};
        EXPECT_EQ(generator.GetCapacity(), 32);
    }
    {
        SpecialSubstitutionGenerator generator{"special:0-1"_special_gen};
        EXPECT_EQ(generator.GetCapacity(), 33);
    }
    {
        SpecialSubstitutionGenerator generator{"special:2"_special_gen};
        EXPECT_EQ(generator.GetCapacity(), 1024);
    }
    {
        SpecialSubstitutionGenerator generator{"special:0-2"_special_gen};
        EXPECT_EQ(generator.GetCapacity(), 1 + 32 + 1024);
    }
}

UTEST(SubstitutionGenerator, SpecialVariableLength) {
    {
        SpecialSubstitutionGenerator generator{"special:0-3"_special_gen};
        auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
        EXPECT_EQ(generator.Generate(seed_hash, 0), ")'#");
        EXPECT_EQ(generator.Generate(seed_hash, 1), "@|_");
        EXPECT_EQ(generator.Generate(seed_hash, 2), "[|*");
        EXPECT_EQ(generator.Generate(seed_hash, 3), "\"'$");

        std::cerr << "Capacity (special:0-3): " << generator.GetCapacity() << std::endl;
        for (std::uint64_t i = 0; i < 1000; ++i) {
            auto value = generator.Generate(seed_hash, i);
            EXPECT_GE(value.size(), 0);
            EXPECT_LE(value.size(), 3);
        }
    }
    {
        SpecialSubstitutionGenerator generator{"special:3-5"_special_gen};
        auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
        EXPECT_EQ(generator.Generate(seed_hash, 0), "*_{\\_");
        EXPECT_EQ(generator.Generate(seed_hash, 1), "~^)?#");

        std::cerr << "Capacity (special:3-5): " << generator.GetCapacity() << std::endl;
        for (std::uint64_t i = 0; i < 1000; ++i) {
            auto value = generator.Generate(seed_hash, i);
            EXPECT_GE(value.size(), 3);
            EXPECT_LE(value.size(), 5);
        }
    }
}

UTEST(PatternGenerator, GetCapacity) {
    PatternGenerator generator(kDictionariesSet, "{noun}"_pattern_ptr);
    EXPECT_EQ(generator.GetCapacity(), 5);
    EXPECT_EQ(generator.GetMaxPatternLength(), 5);

    EXPECT_EQ(PatternGenerator(kDictionariesSet, "{noun}-{noun}"_pattern_ptr).GetCapacity(), 5);
    EXPECT_EQ(PatternGenerator(kDictionariesSet, "{noun}-{noun}-{noun}"_pattern_ptr).GetCapacity(), 5);

    // Mixed case explodes capacity by each word's case space: "nounN" has four letters -> 2^4 = 16
    // cased forms, times 5 nouns = 80 (the digit has no case).
    EXPECT_EQ(PatternGenerator(kDictionariesSet, "{nOun}"_pattern_ptr).GetCapacity(), 80);

    EXPECT_EQ(PatternGenerator(kDictionariesSet, "{adjective}-{noun}"_pattern_ptr).GetCapacity(), 35);
    EXPECT_EQ(PatternGenerator(kDictionariesSet, "{adjective}-{noun}-{noun}"_pattern_ptr).GetCapacity(), 35);

    EXPECT_EQ(PatternGenerator(kDictionariesSet, "{adjective}-{adverb}-{noun}"_pattern_ptr).GetCapacity(), 315);
    EXPECT_EQ(PatternGenerator(kDictionariesSet, "{adjective}-{adverb}-{noun}-{verb}"_pattern_ptr).GetCapacity(), 630);

    EXPECT_EQ(
        PatternGenerator(kDictionariesSet, "{adjective}-{adverb}-{noun}-{number:2d}"_pattern_ptr).GetCapacity(), 6300
    );
    EXPECT_EQ(
        PatternGenerator(kDictionariesSet, "{adjective}-{adverb}-{noun}-{number:2x}"_pattern_ptr).GetCapacity(), 80640
    );
}

UTEST(PatternGenerator, Generate) {
    {
        PatternGenerator generator(kDictionariesSet, "{adjective}-{adverb}-{noun}-{number:2X}"_pattern_ptr);
        auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
        EXPECT_EQ(generator(seed_hash, 0), "adjective6-adverb8-noun1-ED");
        EXPECT_EQ(generator(seed_hash, 1), "adjective3-adverb6-noun5-49");
        EXPECT_EQ(generator(seed_hash, 2), "adjective7-adverb4-noun4-4B");
        EXPECT_EQ(generator(seed_hash, 3), "adjective4-adverb2-noun3-63");
        EXPECT_EQ(generator(seed_hash, 4), "adjective1-adverb9-noun2-73");
    }

    {
        PatternGenerator generator(kDictionariesSet, "{adjective}-{adverb}-{noun@en}-{number:2X}"_pattern_ptr);
        auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
        EXPECT_EQ(generator(seed_hash, 0), "adjective6-adverb8-noun1-ED");
        EXPECT_EQ(generator(seed_hash, 1), "adjective3-adverb6-noun5-49");
        EXPECT_EQ(generator(seed_hash, 2), "adjective7-adverb4-noun4-4B");
        EXPECT_EQ(generator(seed_hash, 3), "adjective4-adverb2-noun3-63");
        EXPECT_EQ(generator(seed_hash, 4), "adjective1-adverb9-noun2-73");
    }
}

UTEST(Generator, GetCapacity) {
    Generator generator(kDictionariesSet);
    EXPECT_EQ(generator.GetCapacity("{noun}").capacity, 5);
    // we are advancing each dictionary each time, so the capacity is lcm of the sizes
    EXPECT_EQ(generator.GetCapacity("{noun}-{noun}").capacity, 5);
    EXPECT_EQ(generator.GetCapacity("{noun}-{noun}-{noun}").capacity, 5);

    EXPECT_EQ(generator.GetCapacity("{adjective}-{noun}").capacity, 35);
    EXPECT_EQ(generator.GetCapacity("{adjective}-{noun}-{noun}").capacity, 35);
    EXPECT_EQ(generator.GetCapacity("{adjective}-{adverb}-{noun}").capacity, 315);
    EXPECT_EQ(generator.GetCapacity("{adjective}-{adverb}-{noun}-{verb}").capacity, 630);

    EXPECT_EQ(generator.GetCapacity("{adjective}-{adverb}-{noun}-{number:2d}").capacity, 6300);
    EXPECT_EQ(generator.GetCapacity("{adjective}-{adverb}-{noun}-{number:2x}").capacity, 80640);
}

UTEST(Generator, GenerateID) {
    Generator generator(kDictionariesSet);
    auto pattern = "-{adjective}-{adverb}-{noun}-{number:2d}-"_pattern_ptr;
    auto settings = generator.GetCapacity(pattern);

    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 0), "-adjective6-adverb8-noun1-53-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 1), "-adjective3-adverb6-noun5-06-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 2), "-adjective7-adverb4-noun4-59-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 3), "-adjective4-adverb2-noun3-12-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 4), "-adjective1-adverb9-noun2-65-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 5), "-adjective5-adverb7-noun1-18-");
}

UTEST(Generator, GenerateIDRoman) {
    Generator generator(kDictionariesSet);
    auto pattern = "-{adjective}-{adverb}-{noun}-{number:2R}-"_pattern_ptr;
    auto settings = generator.GetCapacity(pattern);

    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 0), "-adjective6-adverb8-noun1-DL-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 1), "-adjective3-adverb6-noun5-C-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 2), "-adjective7-adverb4-noun4-IX-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 3), "-adjective4-adverb2-noun3-MV-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 4), "-adjective1-adverb9-noun2-CC-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 5), "-adjective5-adverb7-noun1-XL-");
}

UTEST(Generator, GenerateWithEmoji) {
    // Emoji is a regular file-based dictionary kind now; assemble a set with the fake selector
    // dictionaries plus the real emoji dictionary (decompiled from emoji.bin).
    binary::BinaryDictionary emoji_binary(test::kEmojiTestData);
    std::vector<Dictionary> dicts{
        Dictionary("noun", "en"_lang_view, kNouns),
        Dictionary("adjective", "en"_lang_view, kAdjectives),
        Dictionary("verb", "en"_lang_view, kVerbs),
        Dictionary("adverb", "en"_lang_view, kAdverbs),
        Dictionary("noun", ""_lang_view, kLanguageAgnosticNouns),
    };
    for (auto& d : test::Decompile(emoji_binary)) {
        dicts.push_back(std::move(d));
    }
    Generator generator(DictionarySet{std::move(dicts)});
    auto pattern = "-{emoji:+face}-{adjective}-{adverb}-{noun}-{number:2d}-"_pattern_ptr;
    auto settings = generator.GetCapacity(pattern);

    // The non-emoji placeholders are driven by the fake dictionaries and stay deterministic; only
    // the emoji content depends on the (now file-based) emoji dictionary. Assert the deterministic
    // suffix and that a non-empty emoji was substituted for the leading placeholder.
    const std::vector<std::string> suffixes = {
        "-adjective3-adverb3-noun4-36-", "-adjective5-adverb4-noun2-73-",
        "-adjective7-adverb5-noun5-10-", "-adjective2-adverb6-noun3-47-",
        "-adjective4-adverb7-noun1-84-", "-adjective6-adverb8-noun4-21-",
    };
    for (std::size_t i = 0; i < suffixes.size(); ++i) {
        auto out = generator.Generate(settings, pattern, kTestSeed, i);
        EXPECT_TRUE(out.starts_with("-")) << out;
        EXPECT_TRUE(out.ends_with(suffixes[i])) << "seq " << i << ": " << out;
        // "-<emoji>" precedes the suffix, so there must be at least one emoji byte.
        EXPECT_GT(out.size(), suffixes[i].size() + 1) << out;
    }
}

UTEST(PatternGenerator, Alternation) {
    // Capacity is the sum of the children's capacities (noun=5, verb=10, adjective=7).
    EXPECT_EQ(PatternGenerator(kDictionariesSet, "{noun}|{verb}"_pattern_ptr).GetCapacity(), 15);
    EXPECT_EQ(PatternGenerator(kDictionariesSet, "{noun}|{adjective}"_pattern_ptr).GetCapacity(), 12);
    EXPECT_EQ(PatternGenerator(kDictionariesSet, "{noun}|{verb}|{adjective}"_pattern_ptr).GetCapacity(), 22);
    // Surrounding text keeps a single alternation placeholder.
    EXPECT_EQ(PatternGenerator(kDictionariesSet, "x-{noun}|{verb}-y"_pattern_ptr).GetCapacity(), 15);

    // Golden: exact deterministic output over the full cycle (seed "foobar"), and collision-free
    // (all 15 distinct: five nouns interleaved with ten verbs).
    PatternGenerator generator(kDictionariesSet, "{noun}|{verb}"_pattern_ptr);
    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
    const std::vector<std::string> kExpected = {
        "verb7", "noun1", "verb10", "noun4", "verb3",  "noun2", "verb6", "verb5",
        "verb9", "verb8", "verb2",  "verb1", "noun5",  "verb4", "noun3",
    };
    std::set<std::string> seen;
    for (std::uint64_t i = 0; i < kExpected.size(); ++i) {
        auto slug = generator(seed_hash, i);
        EXPECT_EQ(slug, kExpected[i]) << "seq " << i;
        seen.insert(slug);
    }
    EXPECT_EQ(seen.size(), kExpected.size());
}

UTEST(PatternGenerator, AlternationCollapseAndDisjoint) {
    // Equivalent alternatives collapse -> no double counting.
    EXPECT_EQ(PatternGenerator(kDictionariesSet, "{noun}|{noun}"_pattern_ptr).GetCapacity(), 5);
    EXPECT_EQ(PatternGenerator(kDictionariesSet, "{noun}|{noun}|{adjective}"_pattern_ptr).GetCapacity(), 12);
    // Collapse is type-agnostic: identical number generators collapse; distinct disjoint ones are
    // kept; overlapping ones (decimal vs hex 2-digit share e.g. "27") are an error.
    EXPECT_EQ(PatternGenerator(kDictionariesSet, "{number:2d}|{number:2d}"_pattern_ptr).GetCapacity(), 100);
    EXPECT_EQ(PatternGenerator(kDictionariesSet, "{number:2d}|{number:3d}"_pattern_ptr).GetCapacity(), 1100);
    EXPECT_THROW(PatternGenerator(kDictionariesSet, "{number:2d}|{number:2x}"_pattern_ptr), PatternSyntaxError);
    // Overlapping (non-equivalent) alternatives are a pattern error: {noun} is a superset of
    // {noun:+tag1}, so their outputs intersect.
    EXPECT_THROW(PatternGenerator(kDictionariesSet, "{noun:+tag1}|{noun}"_pattern_ptr), PatternSyntaxError);
    // Disjoint dictionaries are fine (noun* vs verb* never coincide).
    EXPECT_NO_THROW(PatternGenerator(kDictionariesSet, "{noun}|{verb}"_pattern_ptr));
}

}  // namespace slugkit::generator
