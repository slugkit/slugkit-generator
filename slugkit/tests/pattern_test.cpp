#include <slugkit/generator/pattern.hpp>

#include <userver/utest/utest.hpp>

#include <slugkit/generator/exceptions.hpp>

namespace slugkit::generator {

UTEST(PlaceholdersParser, NoPlaceholders) {
    auto placeholders = ParsePlaceholders("test");
    EXPECT_EQ(placeholders.size(), 0);
    placeholders = ParsePlaceholders("test\\{\\}");
    EXPECT_EQ(placeholders.size(), 0);
}

UTEST(PlaceholdersParser, InvalidPlaceholder) {
    EXPECT_THROW(ParsePlaceholders("{number}"), PatternSyntaxError);
    EXPECT_THROW(ParsePlaceholders("{number:5"), PatternSyntaxError);
    EXPECT_THROW(ParsePlaceholders("{number:5,hex"), PatternSyntaxError);
    EXPECT_THROW(ParsePlaceholders("{number:5,hex,dec}"), PatternSyntaxError);
    EXPECT_THROW(ParsePlaceholders("{number:5,h}"), PatternSyntaxError);
    EXPECT_THROW(ParsePlaceholders("}"), PatternSyntaxError);
    EXPECT_THROW(ParsePlaceholders("{selector:=10}"), PatternSyntaxError);
    EXPECT_THROW(ParsePlaceholders("{selector:==}"), PatternSyntaxError);
    EXPECT_THROW(ParsePlaceholders("{selector}[@en]tail"), PatternSyntaxError);
    EXPECT_THROW(ParsePlaceholders("{number:0}"), PatternSyntaxError);
    EXPECT_THROW(ParsePlaceholders("{number:1000d}"), PatternSyntaxError);
    EXPECT_THROW(ParsePlaceholders("{special:0}"), PatternSyntaxError);
    EXPECT_THROW(ParsePlaceholders("{special:1-0}"), PatternSyntaxError);
    EXPECT_THROW(ParsePlaceholders("{special:1-1000}"), PatternSyntaxError);
}

UTEST(PlaceholdersParser, NumberPlaceholder) {
    auto placeholders = ParsePlaceholders("test{number:10}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).max_length, 10);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).base, NumberBase::kDec);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).ToString(), "number:10d");

    placeholders = ParsePlaceholders("test{num:10}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).max_length, 10);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).base, NumberBase::kDec);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).ToString(), "number:10d");

    placeholders = ParsePlaceholders("test{number:10,dec}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).max_length, 10);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).base, NumberBase::kDec);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).ToString(), "number:10d");

    placeholders = ParsePlaceholders("test{number:10d}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).max_length, 10);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).base, NumberBase::kDec);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).ToString(), "number:10d");

    placeholders = ParsePlaceholders("test{number:2,hex}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).max_length, 2);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).base, NumberBase::kHex);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).ToString(), "number:2x");

    placeholders = ParsePlaceholders("test{number:2x}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).max_length, 2);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).base, NumberBase::kHex);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).ToString(), "number:2x");

    placeholders = ParsePlaceholders("test{number:2,HEX}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).max_length, 2);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).base, NumberBase::kHexUpper);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).ToString(), "number:2X");

    placeholders = ParsePlaceholders("test{number:2X}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).max_length, 2);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).base, NumberBase::kHexUpper);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).ToString(), "number:2X");

    placeholders = ParsePlaceholders("test{number:2r}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).max_length, 2);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).base, NumberBase::kRomanLower);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).ToString(), "number:2r");

    placeholders = ParsePlaceholders("test{number:2R}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).max_length, 2);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).base, NumberBase::kRoman);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).ToString(), "number:2R");

    placeholders = ParsePlaceholders("test{number:2,roman}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).max_length, 2);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).base, NumberBase::kRomanLower);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).ToString(), "number:2r");

    placeholders = ParsePlaceholders("test{number:2,ROMAN}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).max_length, 2);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).base, NumberBase::kRoman);
    EXPECT_EQ(std::get<NumberGen>(placeholders[0]).ToString(), "number:2R");
}

UTEST(PlaceholdersParser, SpecialCharPlaceholder) {
    auto placeholders = ParsePlaceholders("test{special:1}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<SpecialCharGen>(placeholders[0]).min_length, 1);
    EXPECT_EQ(std::get<SpecialCharGen>(placeholders[0]).max_length, 1);
    EXPECT_EQ(std::get<SpecialCharGen>(placeholders[0]).ToString(), "special:1");

    placeholders = ParsePlaceholders("test{spec:1}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<SpecialCharGen>(placeholders[0]).min_length, 1);
    EXPECT_EQ(std::get<SpecialCharGen>(placeholders[0]).max_length, 1);
    EXPECT_EQ(std::get<SpecialCharGen>(placeholders[0]).ToString(), "special:1");

    placeholders = ParsePlaceholders("test{special:1-2}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<SpecialCharGen>(placeholders[0]).min_length, 1);
    EXPECT_EQ(std::get<SpecialCharGen>(placeholders[0]).max_length, 2);
    EXPECT_EQ(std::get<SpecialCharGen>(placeholders[0]).ToString(), "special:1-2");

    placeholders = ParsePlaceholders("test{special:1-1}");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<SpecialCharGen>(placeholders[0]).min_length, 1);
    EXPECT_EQ(std::get<SpecialCharGen>(placeholders[0]).max_length, 1);
    EXPECT_EQ(std::get<SpecialCharGen>(placeholders[0]).ToString(), "special:1");
}

UTEST(PlaceholdersParser, SelectorPlaceholderNoModifiers) {
    auto placeholders = ParsePlaceholders("test-{selector}-slug");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).size_limit, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).options, Selector::option_map_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).ToString(), "selector");
}

UTEST(PlaceholdersParser, SelectorPlaceholderLanguage) {
    // Language
    auto placeholders = ParsePlaceholders("test-{selector@en}-slug");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, "en");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).size_limit, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).options, (Selector::option_map_t{}));
    EXPECT_EQ(std::get<Selector>(placeholders[0]).ToString(), "selector@en");
}

UTEST(PlaceholdersParser, SelectorPlaceholderIncludeTags) {
    // Include tags
    auto placeholders = ParsePlaceholders("test-{selector:+tag1+tag2}-slug");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, (Selector::tag_list_t{"tag1", "tag2"}));
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).size_limit, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).ToString(), "selector:+tag1+tag2");

    // Include tags (add whitespace)
    placeholders = ParsePlaceholders("test-{ selector : +tag1 +tag2 }-slug");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, (Selector::tag_list_t{"tag1", "tag2"}));
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).size_limit, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).ToString(), "selector:+tag1+tag2");
}

UTEST(PlaceholdersParser, SelectorPlaceholderExcludeTags) {
    // Exclude tags
    auto placeholders = ParsePlaceholders("test-{selector:-tag1-tag2}-slug");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, (Selector::tag_list_t{"tag1", "tag2"}));
    EXPECT_EQ(std::get<Selector>(placeholders[0]).size_limit, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).ToString(), "selector:-tag1-tag2");

    // Exclude tags (add whitespace)
    placeholders = ParsePlaceholders("test-{ selector : -tag1 -tag2 }-slug");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, (Selector::tag_list_t{"tag1", "tag2"}));
    EXPECT_EQ(std::get<Selector>(placeholders[0]).size_limit, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).ToString(), "selector:-tag1-tag2");
}

UTEST(PlaceholdersParser, SelectorPlaceholderMixedTags) {
    auto placeholders = ParsePlaceholders("test-{selector:+tag1-tag2+tag3-tag4}-slug");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, (Selector::tag_list_t{"tag1", "tag3"}));
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, (Selector::tag_list_t{"tag2", "tag4"}));
    EXPECT_EQ(std::get<Selector>(placeholders[0]).ToString(), "selector:+tag1+tag3-tag2-tag4");
}

UTEST(PlaceholdersParser, SelectorPlaceholderSizeLimit) {
    auto placeholders = ParsePlaceholders("test-{selector:<=10}-slug");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).size_limit, (SizeLimit{CompareOperator::kLe, 10}));
    EXPECT_EQ(std::get<Selector>(placeholders[0]).ToString(), "selector:<=10");

    placeholders = ParsePlaceholders("test-{selector:<10}-slug");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).size_limit, (SizeLimit{CompareOperator::kLt, 10}));
    EXPECT_EQ(std::get<Selector>(placeholders[0]).ToString(), "selector:<10");

    placeholders = ParsePlaceholders("test-{selector:>=10}-slug");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).size_limit, (SizeLimit{CompareOperator::kGe, 10}));
    EXPECT_EQ(std::get<Selector>(placeholders[0]).ToString(), "selector:>=10");

    placeholders = ParsePlaceholders("test-{selector:>10}-slug");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).size_limit, (SizeLimit{CompareOperator::kGt, 10}));
    EXPECT_EQ(std::get<Selector>(placeholders[0]).ToString(), "selector:>10");

    placeholders = ParsePlaceholders("test-{selector:==10}-slug");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).size_limit, (SizeLimit{CompareOperator::kEq, 10}));
    EXPECT_EQ(std::get<Selector>(placeholders[0]).ToString(), "selector:==10");

    placeholders = ParsePlaceholders("test-{selector:!=10}-slug");
    EXPECT_EQ(placeholders.size(), 1);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, std::nullopt);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, Selector::tag_list_t{});
    EXPECT_EQ(std::get<Selector>(placeholders[0]).size_limit, (SizeLimit{CompareOperator::kNe, 10}));
    EXPECT_EQ(std::get<Selector>(placeholders[0]).ToString(), "selector:!=10");
}

UTEST(PlaceholdersParser, MultiplePlaceholders) {
    auto placeholders = ParsePlaceholders("test-{selector}-{number:10}-{selector}-{number:10x}");
    EXPECT_EQ(placeholders.size(), 4);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, std::nullopt);
    EXPECT_EQ(std::get<NumberGen>(placeholders[1]).max_length, 10);
    EXPECT_EQ(std::get<Selector>(placeholders[2]).kind, "selector");
    EXPECT_EQ(std::get<Selector>(placeholders[2]).language, std::nullopt);
    EXPECT_EQ(std::get<NumberGen>(placeholders[3]).max_length, 10);
    EXPECT_EQ(std::get<NumberGen>(placeholders[3]).base, NumberBase::kHex);
}

UTEST(PlaceholderParser, GlobalLanguage) {
    auto placeholders = ParsePlaceholders("test-{selector}-{selector}-{selector}-slug[@en]");
    EXPECT_EQ(placeholders.size(), 3);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, "en");
    EXPECT_EQ(std::get<Selector>(placeholders[1]).language, "en");
    EXPECT_EQ(std::get<Selector>(placeholders[2]).language, "en");

    placeholders = ParsePlaceholders("test-{selector@fr}-{selector}-{selector}-slug[@en]");
    EXPECT_EQ(placeholders.size(), 3);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).language, "fr");
    EXPECT_EQ(std::get<Selector>(placeholders[1]).language, "en");
    EXPECT_EQ(std::get<Selector>(placeholders[2]).language, "en");
}

UTEST(PlaceholderParser, GlobalIncludeTags) {
    auto placeholders = ParsePlaceholders("test-{selector}-{selector}-{selector}-slug[+tag1+tag2]");
    EXPECT_EQ(placeholders.size(), 3);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags.size(), 2);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, (Selector::tag_list_t{"tag1", "tag2"}));
    EXPECT_EQ(std::get<Selector>(placeholders[1]).include_tags.size(), 2);
    EXPECT_EQ(std::get<Selector>(placeholders[1]).include_tags, (Selector::tag_list_t{"tag1", "tag2"}));
    EXPECT_EQ(std::get<Selector>(placeholders[2]).include_tags.size(), 2);
    EXPECT_EQ(std::get<Selector>(placeholders[2]).include_tags, (Selector::tag_list_t{"tag1", "tag2"}));

    placeholders = ParsePlaceholders("test-{selector:+tagN}-{selector}-{selector}-slug[+tag1+tag2]");
    EXPECT_EQ(placeholders.size(), 3);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags.size(), 3);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).include_tags, (Selector::tag_list_t{"tag1", "tag2", "tagN"}));
    EXPECT_EQ(std::get<Selector>(placeholders[1]).include_tags.size(), 2);
    EXPECT_EQ(std::get<Selector>(placeholders[1]).include_tags, (Selector::tag_list_t{"tag1", "tag2"}));
    EXPECT_EQ(std::get<Selector>(placeholders[2]).include_tags.size(), 2);
    EXPECT_EQ(std::get<Selector>(placeholders[2]).include_tags, (Selector::tag_list_t{"tag1", "tag2"}));
}

UTEST(PlaceholderParser, GlobalExcludeTags) {
    auto placeholders = ParsePlaceholders("test-{selector}-{selector}-{selector}-slug[-tag1-tag2]");
    EXPECT_EQ(placeholders.size(), 3);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags.size(), 2);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, (Selector::tag_list_t{"tag1", "tag2"}));
    EXPECT_EQ(std::get<Selector>(placeholders[1]).exclude_tags.size(), 2);
    EXPECT_EQ(std::get<Selector>(placeholders[1]).exclude_tags, (Selector::tag_list_t{"tag1", "tag2"}));
    EXPECT_EQ(std::get<Selector>(placeholders[2]).exclude_tags.size(), 2);
    EXPECT_EQ(std::get<Selector>(placeholders[2]).exclude_tags, (Selector::tag_list_t{"tag1", "tag2"}));

    placeholders = ParsePlaceholders("test-{selector:-tagN}-{selector}-{selector}-slug[-tag1-tag2]");
    EXPECT_EQ(placeholders.size(), 3);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags.size(), 3);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).exclude_tags, (Selector::tag_list_t{"tag1", "tag2", "tagN"}));
    EXPECT_EQ(std::get<Selector>(placeholders[1]).exclude_tags.size(), 2);
    EXPECT_EQ(std::get<Selector>(placeholders[1]).exclude_tags, (Selector::tag_list_t{"tag1", "tag2"}));
    EXPECT_EQ(std::get<Selector>(placeholders[2]).exclude_tags.size(), 2);
    EXPECT_EQ(std::get<Selector>(placeholders[2]).exclude_tags, (Selector::tag_list_t{"tag1", "tag2"}));
}

UTEST(PlaceholderParser, GlobalSizeLimit) {
    auto placeholders = ParsePlaceholders("test-{selector}-{selector}-{selector}-slug[<=10]");
    EXPECT_EQ(placeholders.size(), 3);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).size_limit, (SizeLimit{CompareOperator::kLe, 10}));
    EXPECT_EQ(std::get<Selector>(placeholders[1]).size_limit, (SizeLimit{CompareOperator::kLe, 10}));
    EXPECT_EQ(std::get<Selector>(placeholders[2]).size_limit, (SizeLimit{CompareOperator::kLe, 10}));

    placeholders = ParsePlaceholders("test-{selector:<=8}-{selector}-{selector}-slug[<=10]");
    EXPECT_EQ(placeholders.size(), 3);
    EXPECT_EQ(std::get<Selector>(placeholders[0]).size_limit, (SizeLimit{CompareOperator::kLe, 8}));
    EXPECT_EQ(std::get<Selector>(placeholders[1]).size_limit, (SizeLimit{CompareOperator::kLe, 10}));
    EXPECT_EQ(std::get<Selector>(placeholders[2]).size_limit, (SizeLimit{CompareOperator::kLe, 10}));
}

UTEST(PatternParser, NoPlaceholders) {
    {
        auto pattern = ParsePattern("test");
        EXPECT_EQ(pattern.placeholders.size(), 0);
        EXPECT_EQ(pattern.text_chunks.size(), 1);
        EXPECT_EQ(pattern.text_chunks[0], "test");
        EXPECT_EQ(pattern.ArbitraryTextLength(), 4);
        EXPECT_EQ(pattern.ToString(), "test");
    }

    {
        auto pattern = ParsePattern("");
        EXPECT_EQ(pattern.placeholders.size(), 0);
        EXPECT_EQ(pattern.text_chunks.size(), 1);
        EXPECT_EQ(pattern.text_chunks[0], "");
        EXPECT_EQ(pattern.ArbitraryTextLength(), 0);
        EXPECT_EQ(pattern.ToString(), "");
    }
}

UTEST(PatternParser, SimplePlaceholders) {
    {
        auto pattern = ParsePattern("-{selector}-");
        EXPECT_EQ(pattern.placeholders.size(), 1);
        EXPECT_EQ(pattern.text_chunks.size(), 2);
        EXPECT_EQ(pattern.text_chunks[0], "-");
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).kind, "selector");
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).language, std::nullopt);
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).include_tags, Selector::tag_list_t{});
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).exclude_tags, Selector::tag_list_t{});
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).size_limit, std::nullopt);
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).options, (Selector::option_map_t{}));
        EXPECT_EQ(pattern.text_chunks[1], "-");
        EXPECT_EQ(pattern.ArbitraryTextLength(), 2);
        EXPECT_EQ(pattern.ToString(), "-{selector}-");
    }
    {
        auto pattern = ParsePattern("- kebab {selector} -");
        EXPECT_EQ(pattern.placeholders.size(), 1);
        EXPECT_EQ(pattern.text_chunks.size(), 2);
        EXPECT_EQ(pattern.text_chunks[0], "- kebab ");
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).kind, "selector");
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).language, std::nullopt);
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).include_tags, Selector::tag_list_t{});
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).exclude_tags, Selector::tag_list_t{});
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).size_limit, std::nullopt);
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).options, (Selector::option_map_t{}));
        EXPECT_EQ(pattern.text_chunks[1], " -");
        EXPECT_EQ(pattern.ArbitraryTextLength(), 10);
        EXPECT_EQ(pattern.ToString(), "- kebab {selector} -");
    }
    {
        auto pattern = ParsePattern("{selector}");
        EXPECT_EQ(pattern.placeholders.size(), 1);
        EXPECT_EQ(pattern.text_chunks.size(), 2);
        EXPECT_EQ(pattern.text_chunks[0], "");
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).kind, "selector");
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).language, std::nullopt);
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).include_tags, Selector::tag_list_t{});
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).exclude_tags, Selector::tag_list_t{});
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).size_limit, std::nullopt);
        EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).options, (Selector::option_map_t{}));
        EXPECT_EQ(pattern.text_chunks[1], "");
        EXPECT_EQ(pattern.ArbitraryTextLength(), 0);
        EXPECT_EQ(pattern.ToString(), "{selector}");
    }

    {
        auto pattern = ParsePattern("-{number:10}-");
        EXPECT_EQ(pattern.placeholders.size(), 1);
        EXPECT_EQ(pattern.text_chunks.size(), 2);
        EXPECT_EQ(pattern.text_chunks[0], "-");
        EXPECT_EQ(std::get<NumberGen>(pattern.placeholders[0]).max_length, 10);
        EXPECT_EQ(std::get<NumberGen>(pattern.placeholders[0]).base, NumberBase::kDec);
        EXPECT_EQ(pattern.text_chunks[1], "-");
        EXPECT_EQ(pattern.ArbitraryTextLength(), 2);
        EXPECT_EQ(pattern.ToString(), "-{number:10d}-");
    }
}

UTEST(PatternParser, GlobalLanguage) {
    auto pattern = ParsePattern("test-{selector}-{selector}-{selector}-slug[@en]");
    EXPECT_EQ(pattern.placeholders.size(), 3);
    EXPECT_EQ(std::get<Selector>(pattern.placeholders[0]).language, "en");
    EXPECT_EQ(std::get<Selector>(pattern.placeholders[1]).language, "en");
    EXPECT_EQ(std::get<Selector>(pattern.placeholders[2]).language, "en");

    EXPECT_EQ(pattern.text_chunks.size(), pattern.placeholders.size() + 1);
    EXPECT_EQ(pattern.text_chunks[0], "test-");
    EXPECT_EQ(pattern.text_chunks[1], "-");
    EXPECT_EQ(pattern.text_chunks[2], "-");
    EXPECT_EQ(pattern.text_chunks[3], "-slug");

    EXPECT_EQ(pattern.ToString(), "test-{selector@en}-{selector@en}-{selector@en}-slug");
}

UTEST(PatternLiterals, Selector) {
    using namespace literals;
    auto selector = "selector@en:+tag1-tag2 <=10"_selector;
    EXPECT_EQ(selector.kind, "selector");
    EXPECT_EQ(selector.language, "en");
    EXPECT_EQ(selector.include_tags, (Selector::tag_list_t{"tag1"}));
    EXPECT_EQ(selector.exclude_tags, (Selector::tag_list_t{"tag2"}));
    EXPECT_EQ(selector.size_limit, (SizeLimit{CompareOperator::kLe, 10}));
    EXPECT_EQ(selector.ToString(), "selector@en:+tag1-tag2<=10");

    EXPECT_THROW("number:10h"_selector, PatternSyntaxError);
}

UTEST(PatternLiterals, NumberGen) {
    using namespace literals;
    auto number = "number:10d"_number_gen;
    EXPECT_EQ(number.max_length, 10);
    EXPECT_EQ(number.base, NumberBase::kDec);

    EXPECT_THROW("noun@en:+tag1-tag2 <=10"_number_gen, PatternSyntaxError);
    EXPECT_THROW("number:10,h"_number_gen, PatternSyntaxError);
}

UTEST(SlugFormatter, Empty) {
    auto pattern = ParsePattern("");
    auto formatter = SlugFormatter(pattern);
    EXPECT_EQ(formatter({}), "");

    EXPECT_THROW(formatter({""}), SlugFormatError);
}

UTEST(SlugFormatter, Simple) {
    auto pattern = ParsePattern("-{selector}-");
    auto formatter = SlugFormatter(pattern);
    EXPECT_EQ(formatter({"test"}), "-test-");

    EXPECT_THROW(formatter({}), SlugFormatError);
}

UTEST(SlugFormatter, Multiple) {
    auto pattern = ParsePattern("~{selector}-{number:10}-{selector}-{number:4x}~");
    auto formatter = SlugFormatter(pattern);
    EXPECT_EQ(formatter({"test", "1234567890", "bla", "ffa0"}), "~test-1234567890-bla-ffa0~");

    EXPECT_THROW(formatter({"test"}), SlugFormatError);
}

}  // namespace slugkit::generator
