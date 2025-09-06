#include <slugkit/generator/exceptions.hpp>
#include <slugkit/generator/generator.hpp>
#include <slugkit/generator/pattern_generator.hpp>
#include <slugkit/utils/text.hpp>

#include <userver/utest/utest.hpp>

#include <iostream>

namespace slugkit::generator {

using namespace literals;

namespace {

const std::vector<Word> kNouns = {
    {"noun1", {}},
    {"noun2", {}},
    {"noun3", {"tag1"}},
    {"noun4", {"tag2", "nsfw"}},
    {"noun5", {"tag1", "tag2"}},
};

const std::vector<Word> kAdjectives = {
    {"adjective1", {}},
    {"adjective2", {}},
    {"adjective3", {"tag1"}},
    {"adjective4", {"tag2", "nsfw"}},
    {"adjective5", {"tag1", "tag2"}},
    {"adjective6", {"tag1", "tag2", "nsfw"}},
    {"adjective7", {"tag1", "tag2", "nsfw"}},
};

const std::vector<Word> kVerbs = {
    {"verb1", {}},
    {"verb2", {}},
    {"verb3", {"tag1"}},
    {"verb4", {"tag2", "nsfw"}},
    {"verb5", {"tag1", "tag2"}},
    {"verb6", {"tag1", "tag2", "nsfw"}},
    {"verb7", {"tag1", "tag2", "nsfw"}},
    {"verb8", {"tag1", "tag2", "nsfw"}},
    {"verb9", {"tag1", "tag2", "nsfw"}},
    {"verb10", {"tag1", "tag2", "nsfw"}},
};

const std::vector<Word> kAdverbs = {
    {"adverb1", {}},
    {"adverb2", {}},
    {"adverb3", {"tag1"}},
    {"adverb4", {"tag2", "nsfw"}},
    {"adverb5", {"tag1", "tag2"}},
    {"adverb6", {"tag1", "tag2", "nsfw"}},
    {"adverb7", {"tag1", "tag2", "nsfw"}},
    {"adverb8", {"tag1", "tag2", "nsfw"}},
    {"adverb9", {"tag1", "tag2", "nsfw"}},
};

const std::vector<Word> kLanguageAgnosticNouns = {
    {"noun1", {}},
    {"noun2", {}},
    {"noun3", {"tag1"}},
    {"noun4", {"tag2", "nsfw"}},
    {"noun5", {"tag1", "tag2"}},
};

const std::map<std::string, Dictionary> kDictionaries = {
    {"noun", Dictionary("noun", "en", kNouns)},
    {"adjective", Dictionary("adjective", "en", kAdjectives)},
    {"verb", Dictionary("verb", "en", kVerbs)},
    {"adverb", Dictionary("adverb", "en", kAdverbs)},
};

const DictionarySet kDictionariesSet{
    {
        Dictionary("noun", "en", kNouns),
        Dictionary("adjective", "en", kAdjectives),
        Dictionary("verb", "en", kVerbs),
        Dictionary("adverb", "en", kAdverbs),
        Dictionary("noun", "", kLanguageAgnosticNouns),
    },
};

const Selector kNounSelector = "noun"_selector;
const Selector kEnNounSelector = "noun@en"_selector;
constexpr auto kTestSeed = "foobar";

}  // namespace

UTEST(SubstitutionGenerator, LowerCaseWords) {
    Dictionary dictionary("noun", "en", kNouns);
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
    Dictionary dictionary("noun", "en", kNouns);

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
    Dictionary dictionary("noun", "en", kNouns);

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
    Dictionary dictionary("noun", "en", kNouns);

    auto filtered_dictionary = dictionary.Filter("nOun"_selector);
    SelectorSubstitutionGenerator generator{filtered_dictionary, {5, 5}};
    auto seed_hash = PatternGenerator::SeedHash(kTestSeed);
    EXPECT_EQ(generator.Generate(seed_hash, 0), "NOUn2");
    EXPECT_EQ(generator.Generate(seed_hash, 1), "Noun3");
    EXPECT_EQ(generator.Generate(seed_hash, 2), "NOUN4");
    EXPECT_EQ(generator.Generate(seed_hash, 3), "NOun5");
    EXPECT_EQ(generator.Generate(seed_hash, 4), "nouN1");
    EXPECT_EQ(generator.Generate(seed_hash, 5), "NoUN2");
    EXPECT_EQ(generator.Generate(seed_hash, 6), "nOUn3");
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
    Generator generator(kDictionariesSet);
    auto pattern = "-{emoji:+face}-{adjective}-{adverb}-{noun}-{number:2d}-"_pattern_ptr;
    auto settings = generator.GetCapacity(pattern);

    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 0), "-😇-adjective3-adverb3-noun4-36-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 1), "-😜-adjective5-adverb4-noun2-73-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 2), "-😏-adjective7-adverb5-noun5-10-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 3), "-😫-adjective2-adverb6-noun3-47-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 4), "-🤕-adjective4-adverb7-noun1-84-");
    EXPECT_EQ(generator.Generate(settings, pattern, kTestSeed, 5), "-😮-adjective6-adverb8-noun4-21-");
}

}  // namespace slugkit::generator
