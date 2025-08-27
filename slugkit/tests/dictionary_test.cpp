#include <slugkit/generator/dictionary.hpp>

#include <userver/utest/utest.hpp>

namespace slugkit::generator {

using namespace literals;

namespace {

const std::vector<Word> kNouns = {
    {"noun1", {}},
    {"noun2", {}},
    {"noun3", {"tag1"}},
    {"noun4", {"tag2", "obscene"}},
    {"noun5", {"tag1", "tag2"}},
};

const std::vector<Word> kAdjectives = {
    {"adjective1", {}},
    {"adjective2", {}},
    {"adjective3", {"tag1"}},
    {"adjective4", {"tag2", "obscene"}},
    {"adjective5", {"tag1", "tag2"}},
    {"adjective6", {"tag1", "tag2", "obscene"}},
    {"adjective7", {"tag1", "tag2", "obscene"}},
};

const std::vector<Word> kVerbs = {
    {"verb1", {}},
    {"verb2", {}},
    {"verb3", {"tag1"}},
    {"verb4", {"tag2", "obscene"}},
    {"verb5", {"tag1", "tag2"}},
    {"verb6", {"tag1", "tag2", "obscene"}},
    {"verb7", {"tag1", "tag2", "obscene"}},
    {"verb8", {"tag1", "tag2", "obscene"}},
    {"verb9", {"tag1", "tag2", "obscene"}},
    {"verb10", {"tag1", "tag2", "obscene"}},
};

const std::vector<Word> kAdverbs = {
    {"adverb1", {}},
    {"adverb2", {}},
    {"adverb3", {"tag1"}},
    {"adverb4", {"tag2", "obscene"}},
    {"adverb5", {"tag1", "tag2"}},
    {"adverb6", {"tag1", "tag2", "obscene"}},
    {"adverb7", {"tag1", "tag2", "obscene"}},
    {"adverb8", {"tag1", "tag2", "obscene"}},
    {"adverb9", {"tag1", "tag2", "obscene"}},
};

const std::map<std::string, Dictionary> kDictionaries = {
    {"noun", Dictionary("noun", "en", kNouns)},
    {"adjective", Dictionary("adjective", "en", kAdjectives)},
    {"verb", Dictionary("verb", "en", kVerbs)},
    {"adverb", Dictionary("adverb", "en", kAdverbs)},
};

void CheckDictionary(const Dictionary& dictionary, const std::vector<Word>& expected_words) {
    ASSERT_EQ(dictionary.size(), expected_words.size());
    ASSERT_EQ(dictionary.empty(), false);
    for (std::size_t i = 0; i < expected_words.size(); ++i) {
        ASSERT_EQ(dictionary[i], expected_words[i].word);
        ASSERT_EQ(dictionary.GetWord(i), expected_words[i]);
    }
}

const Selector kNounSelector = "noun"_selector;
const Selector kEnNounSelector = "noun@en"_selector;

const FilteredDictionaryPtr kEmptyFilter = FilteredDictionaryPtr{nullptr};

}  // namespace

UTEST(Dictionary, Empty) {
    Dictionary dictionary("test", "en", {});
    EXPECT_EQ(dictionary.size(), 0);
    EXPECT_EQ(dictionary.empty(), true);
    EXPECT_EQ(dictionary.Filter({}), kEmptyFilter);
}

UTEST(Dictionary, Filter) {
    Dictionary dictionary("noun", "en", kNouns);

    CheckDictionary(dictionary, kNouns);

    EXPECT_EQ(dictionary.GetKind(), "noun");
    EXPECT_EQ(dictionary.GetLanguage(), "en");

    // Filtering with an empty selector should return an empty dictionary
    EXPECT_EQ(dictionary.Filter({}), kEmptyFilter);

    // Filtering with a selector should return a dictionary with the same size and kind
    EXPECT_EQ(dictionary.Filter(kNounSelector)->size(), kNouns.size());
    EXPECT_EQ(dictionary.Filter(kNounSelector)->empty(), false);
    EXPECT_EQ(dictionary.Filter(kNounSelector)->GetSelector(), kNounSelector);
    EXPECT_EQ(dictionary.Filter(kNounSelector)->GetWord(0), kNouns[0]);

    EXPECT_EQ(dictionary.Filter(kEnNounSelector)->size(), kNouns.size());
    EXPECT_EQ(dictionary.Filter(kEnNounSelector)->empty(), false);
    EXPECT_EQ(dictionary.Filter(kEnNounSelector)->GetSelector(), kEnNounSelector);
    EXPECT_EQ(dictionary.Filter(kEnNounSelector)->GetWord(0), kNouns[0]);
}

UTEST(Dictionary, FilterByIncludeTags) {
    Dictionary dictionary("noun", "en", kNouns);

    CheckDictionary(dictionary, kNouns);

    EXPECT_EQ(dictionary.GetKind(), "noun");
    EXPECT_EQ(dictionary.GetLanguage(), "en");

    auto selector = "noun:+tag1"_selector;
    ASSERT_EQ(selector.include_tags, (Selector::tag_list_t{"tag1"}));

    ASSERT_EQ(dictionary.Filter(selector)->size(), 2);
    EXPECT_EQ(dictionary.Filter(selector)->empty(), false);
    EXPECT_EQ((*dictionary.Filter(selector))[0], kNouns[2].word);
    EXPECT_EQ((*dictionary.Filter(selector))[1], kNouns[4].word);

    selector = "noun:+tag2"_selector;
    ASSERT_EQ(selector.include_tags, (Selector::tag_list_t{"tag2"}));

    ASSERT_EQ(dictionary.Filter(selector)->size(), 2);
    EXPECT_EQ(dictionary.Filter(selector)->empty(), false);
    EXPECT_EQ((*dictionary.Filter(selector))[0], kNouns[3].word);
    EXPECT_EQ((*dictionary.Filter(selector))[1], kNouns[4].word);
}

UTEST(Dictionary, FilterByExcludeTags) {
    Dictionary dictionary("noun", "en", kNouns);
    EXPECT_EQ(dictionary.size(), 5);
    EXPECT_EQ(dictionary.empty(), false);
    EXPECT_EQ(dictionary.GetKind(), "noun");

    auto selector = "noun:-tag1"_selector;
    ASSERT_EQ(selector.exclude_tags, (Selector::tag_list_t{"tag1"}));

    // Filtering with a selector should return a dictionary with the same size and kind
    ASSERT_EQ(dictionary.Filter(selector)->size(), 3);
    EXPECT_EQ(dictionary.Filter(selector)->empty(), false);
    EXPECT_EQ((*dictionary.Filter(selector))[0], kNouns[0].word);
    EXPECT_EQ((*dictionary.Filter(selector))[1], kNouns[1].word);
    EXPECT_EQ((*dictionary.Filter(selector))[2], kNouns[3].word);

    selector = "noun:-tag2"_selector;
    ASSERT_EQ(selector.exclude_tags, (Selector::tag_list_t{"tag2"}));

    ASSERT_EQ(dictionary.Filter(selector)->size(), 3);
    EXPECT_EQ(dictionary.Filter(selector)->empty(), false);
    EXPECT_EQ((*dictionary.Filter(selector))[0], kNouns[0].word);
    EXPECT_EQ((*dictionary.Filter(selector))[1], kNouns[1].word);
    EXPECT_EQ((*dictionary.Filter(selector))[2], kNouns[2].word);
}

UTEST(Dictionary, FilterBySizeLimit) {
    Dictionary dictionary("noun", "en", kNouns);
    CheckDictionary(dictionary, kNouns);

    auto selector = "noun:<3"_selector;
    ASSERT_EQ(selector.size_limit, (SizeLimit{CompareOperator::kLt, 3}));

    ASSERT_NE(dictionary.Filter(selector), kEmptyFilter);
    EXPECT_EQ(dictionary.Filter(selector)->size(), 0);
    EXPECT_EQ(dictionary.Filter(selector)->empty(), true);

    selector = "noun:<=3"_selector;
    ASSERT_EQ(selector.size_limit, (SizeLimit{CompareOperator::kLe, 3}));

    ASSERT_NE(dictionary.Filter(selector), kEmptyFilter);
    EXPECT_EQ(dictionary.Filter(selector)->size(), 0);
    EXPECT_EQ(dictionary.Filter(selector)->empty(), true);

    selector = "noun:>3"_selector;
    ASSERT_EQ(selector.size_limit, (SizeLimit{CompareOperator::kGt, 3}));

    ASSERT_EQ(dictionary.Filter(selector)->size(), dictionary.size());
    EXPECT_EQ(dictionary.Filter(selector)->empty(), false);

    selector = "noun:>=3"_selector;
    ASSERT_EQ(selector.size_limit, (SizeLimit{CompareOperator::kGe, 3}));

    ASSERT_EQ(dictionary.Filter(selector)->size(), dictionary.size());
    EXPECT_EQ(dictionary.Filter(selector)->empty(), false);

    selector = "noun:==5"_selector;
    ASSERT_EQ(selector.size_limit, (SizeLimit{CompareOperator::kEq, 5}));

    ASSERT_EQ(dictionary.Filter(selector)->size(), dictionary.size());
    EXPECT_EQ(dictionary.Filter(selector)->empty(), false);

    selector = "noun:!=5"_selector;
    ASSERT_EQ(selector.size_limit, (SizeLimit{CompareOperator::kNe, 5}));

    ASSERT_NE(dictionary.Filter(selector), kEmptyFilter);
    EXPECT_EQ(dictionary.Filter(selector)->size(), 0);
    EXPECT_EQ(dictionary.Filter(selector)->empty(), true);
}

UTEST(Dictionary, FilterWithCaseModifier) {
    Dictionary dictionary("noun", "en", kNouns);
    EXPECT_EQ(dictionary.size(), 5);
    EXPECT_EQ(dictionary.empty(), false);
    EXPECT_EQ(dictionary.GetKind(), "noun");
    EXPECT_EQ(dictionary.GetLanguage(), "en");
    EXPECT_EQ(dictionary[0], kNouns[0].word);
    EXPECT_EQ(dictionary.GetWord(0), kNouns[0]);

    // Filtering with a selector should return a dictionary with the same size and kind
    // String case modifier should be ignored when filtering
    // String case should be applied to the result
    auto selector = "Noun@en"_selector;
    EXPECT_EQ(dictionary.Filter(selector)->size(), dictionary.size());
    EXPECT_EQ(dictionary.Filter(selector)->empty(), false);
    EXPECT_EQ((*dictionary.Filter(selector))[0], "Noun1");

    // Filtering with a selector should return a dictionary with the same size and kind
    // String case modifier should be ignored when filtering
    // String case should be applied to the result
    selector = "NOUN@en"_selector;
    EXPECT_EQ(dictionary.Filter(selector)->size(), dictionary.size());
    EXPECT_EQ(dictionary.Filter(selector)->empty(), false);
    EXPECT_EQ((*dictionary.Filter(selector))[0], "NOUN1");
}

}  // namespace slugkit::generator
