#include <slugkit/generator/detail/indexes.hpp>

#include <userver/utest/utest.hpp>

namespace slugkit::generator::detail {

using namespace literals;

const std::vector<Word> kWords = {
    {"1234567890", {"a", "b"}},  // length = 10, tag = a, b
    {"1234", {"a"}},             // length = 4, tag = a
    {"12", {}},                  // length = 2, no tags
    {"12345678901", {"b"}},      // length = 11, tag = b
    {"ab", {"c"}},               // length = 2, tag = c
    {"123456789", {"d"}},        // length = 9, tag = d
    {"12345678", {"e"}},         // length = 8, tag = e
    {"123", {"f"}},              // length = 3, tag = f
    {"123456", {"g", "h"}},      // length = 6, tag = g, h
    {"1234", {"h"}},             // length = 4, tag = h
    {"123456789012", {"i"}},     // length = 12, tag = i
    {"12345", {"j", "k"}},       // length = 5, tag = j, k
    {"1234567", {"k"}},          // length = 7, tag = k
    {"abcdefghi", {"l"}},        // length = 9, tag = l
};

UTEST(LengthIndex, Empty) {
    LengthIndex index;
    EXPECT_EQ(index.MaxLength(), 0);
    EXPECT_EQ(index.Eq(10).words.size(), 0);
    EXPECT_EQ(index.Ne(10).words.size(), 0);
    EXPECT_EQ(index.Lt(10).words.size(), 0);
    EXPECT_EQ(index.Le(10).words.size(), 0);
    EXPECT_EQ(index.Gt(10).words.size(), 0);
    EXPECT_EQ(index.Ge(10).words.size(), 0);
}

UTEST(LengthIndex, NoSizeLimit) {
    LengthIndex index(kWords);
    EXPECT_EQ(index.Query("word"_selector).words.size(), kWords.size());
}

UTEST(LengthIndex, MaxLength) {
    LengthIndex index(kWords);
    EXPECT_EQ(index.MaxLength(), 12);
}

UTEST(LengthIndex, Eq) {
    LengthIndex index(kWords);
    EXPECT_EQ(index.Eq(10).words.size(), 1);
    EXPECT_EQ((*index.Eq(10).words.begin()).second->word, "1234567890");

    EXPECT_EQ(index.Eq(9).words.size(), 2);
}

UTEST(LengthIndex, Ne) {
    LengthIndex index(kWords);
    EXPECT_EQ(index.Ne(10).words.size(), kWords.size() - 1);
}

UTEST(LengthIndex, Le) {
    LengthIndex index(kWords);
    auto result = index.Le(10);
    auto words = result.ToSet();
    EXPECT_EQ(words.size(), kWords.size() - 2);
    EXPECT_EQ((*words.begin())->word, "1234567890");
    EXPECT_EQ(result.MaxLength(), 10);
    EXPECT_EQ(result.MinLength(), 2);
}

UTEST(TagIndex, Empty) {
    TagIndex index;
    EXPECT_EQ(index.Query(Selector{}).size(), 0);
}

UTEST(TagIndex, NoTags) {
    TagIndex index(kWords);
    EXPECT_EQ(index.Query(Selector{}).size(), kWords.size());
}

UTEST(TagIndex, IncludeTags) {
    TagIndex index(kWords);
    EXPECT_EQ(index.Query("word:+a"_selector).size(), 2);
    EXPECT_EQ(index.Query("word:+a +b"_selector).size(), 1);
    EXPECT_EQ(index.Query("word:+a +b +c"_selector).size(), 0);
}

UTEST(TagIndex, ExcludeTags) {
    TagIndex index(kWords);
    EXPECT_EQ(index.Query("word:+a -b"_selector).size(), 1);
    auto has_a = index.Query("word:+a"_selector).size();
    EXPECT_EQ(has_a, 2);
    EXPECT_EQ(index.Query("word:-a"_selector).size(), kWords.size() - has_a);
}

}  // namespace slugkit::generator::detail
