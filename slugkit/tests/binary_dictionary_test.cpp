#include <slugkit/generator/binary_dictionary.hpp>

#include <slugkit/test_utils/test_dictionary.hpp>

#include <slugkit/test_utils/data_config.hpp>

#include <slugkit/utils/memory_mapped_file.hpp>

#include <userver/utest/utest.hpp>

namespace slugkit::generator::binary {

using namespace literals;
using namespace detail::literals;
using namespace generator::literals;

namespace {

constexpr IndexType kTestWordCount{7238};

constexpr char kHeaderRawData[] =
    "SLUGDICT"
    "\x01\0\0\0"             // binary format version, 1 (little-endian 4 bytes)
    "\x00\0"                 // kind markup, start 0 (little-endian 2 bytes)
    "\x04\0"                 // kind markup, length 4 (little-endian 2 bytes)
    "\x04\0"                 // version markup, start 4 (little-endian 2 bytes)
    "\x05\0"                 // version markup, length 5 (little-endian 2 bytes)
    "\x09\0"                 // description markup, start 9 (little-endian 2 bytes)
    "\x15\0"                 // description markup, length 21 (little-endian 2 bytes)
    "noun"                   // kind
    "0.0.1"                  // version
    "dictionary definition"  // description
    ;

const std::span<const std::byte> kHeaderData =
    std::span<const std::byte>(reinterpret_cast<const std::byte*>(kHeaderRawData), sizeof(kHeaderRawData));

constexpr char kWordEntryRawData[] =
    "WORDS==="
    "\0\0"    // lowercase markup, start 0 (little-endian 2 bytes)
    "\x04\0"  // lowercase markup, length 4 (little-endian 2 bytes)
    "\x04\0"  // uppercase markup, start 4 (little-endian 2 bytes)
    "\x04\0"  // uppercase markup, length 4 (little-endian 2 bytes)
    "\x08\0"  // titlecase markup, start 8 (little-endian 2 bytes)
    "\x04\0"  // titlecase markup, length 8 (little-endian 2 bytes)
    "noun"    // lowercase
    "NOUN"    // uppercase
    "Noun"    // titlecase
    ;

const std::span<const std::byte> kWordEntryData =
    std::span<const std::byte>(reinterpret_cast<const std::byte*>(kWordEntryRawData), sizeof(kWordEntryRawData));

}  // namespace

UTEST(BinaryDictionary, DISABLED_Header) {
    const auto* header = detail::Header::GetHeader(kHeaderData);
    EXPECT_TRUE(header->IsValid());
    EXPECT_EQ(header->MagicNum(), "SLUGDICT");
    EXPECT_EQ(header->Size(), SizeType(54));
    EXPECT_EQ(header->Kind(), "noun");
    EXPECT_EQ(header->Version(), "0.0.1");
    EXPECT_EQ(header->Description(), "dictionary definition");
}

UTEST(BinaryDictionary, WordEntry) {
    const auto* word_entry = WordEntry::GetWordEntry(kWordEntryData.subspan(detail::WordData::kMagicNum.size(), 24));
    EXPECT_EQ(word_entry->Lowercase(), "noun");
    EXPECT_EQ(word_entry->Uppercase(), "NOUN");
    EXPECT_EQ(word_entry->Titlecase(), "Noun");
    EXPECT_EQ(word_entry->Size(), 24);
}

UTEST(BinaryDictionary, DISABLED_WordData) {
    const auto* word_data = detail::WordData::GetWordData(kWordEntryData);
    const auto offset = 0_off;
    EXPECT_EQ(word_data->At(offset).Lowercase(), "noun");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "NOUN");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "Noun");
    EXPECT_EQ(word_data->At(offset).Size(), 24);
}

void TestData(std::span<const std::byte> data) {
    // Header
    const auto* header = detail::Header::GetHeader(data);
    EXPECT_TRUE(header->IsValid());
    EXPECT_EQ(header->MagicNum(), detail::Header::kMagicNum);
    EXPECT_EQ(header->Size(), 58);
    EXPECT_EQ(header->Kind(), "adverb");
    EXPECT_EQ(header->Version(), "1.0.0");
    EXPECT_EQ(header->Description(), "A dictionary of adverbs");

    auto consumed_size = detail::Align(header->Size()).GetUnderlying();

    // IndexTable
    const auto* index_table = detail::IndexTable::GetIndexTable(data.subspan(consumed_size));
    EXPECT_TRUE(index_table->IsValid());
    EXPECT_EQ(index_table->MagicNum(), detail::IndexTable::kMagicNum);
    EXPECT_EQ(index_table->Count(), kTestWordCount);
    EXPECT_EQ((*index_table)[IndexType(0)], 0);
    EXPECT_EQ((*index_table)[IndexType(1)], 18);
    EXPECT_EQ((*index_table)[IndexType(2)], 36);
    EXPECT_EQ((*index_table)[IndexType(3)], 54);

    consumed_size += detail::Align(index_table->Size()).GetUnderlying();

    // LanguageTable
    const auto* language_table = detail::LanguageTable::GetLanguageTable(data.subspan(consumed_size));
    EXPECT_TRUE(language_table->IsValid());
    EXPECT_EQ(language_table->MagicNum(), detail::LanguageTable::kMagicNum);
    EXPECT_EQ(language_table->Count(), 2);

    EXPECT_EQ(language_table->Languages(), (LanguageCodeSet{"en"_lang_view, "fr"_lang_view}));

    EXPECT_EQ((*language_table)[0_idx].Code(), "en");
    EXPECT_EQ((*language_table)[0_idx].Count(), kTestWordCount / 2);
    EXPECT_EQ((*language_table)[1_idx].Code(), "fr");
    EXPECT_EQ((*language_table)[1_idx].Count(), kTestWordCount / 2);

    consumed_size += detail::Align(language_table->Size()).GetUnderlying();

    // TagsTable
    const auto* tags_table = detail::TagsTable::GetTagsTable(data.subspan(consumed_size));
    EXPECT_TRUE(tags_table->IsValid());
    EXPECT_EQ(tags_table->MagicNum(), detail::TagsTable::kMagicNum);
    EXPECT_EQ(tags_table->Count(), 7);
    EXPECT_EQ(
        tags_table->Tags(),
        (TagSet{
            "det"_tag_view,
            "emo"_tag_view,
            "neg"_tag_view,
            "neut"_tag_view,
            "nsfw"_tag_view,
            "obj"_tag_view,
            "pos"_tag_view
        })
    );

    EXPECT_EQ((*tags_table)[0_idx].Name(), "det");
    EXPECT_FALSE((*tags_table)[0_idx].OptIn());
    EXPECT_EQ((*tags_table)[0_idx].Count(), 6952);

    EXPECT_EQ((*tags_table)[1_idx].Name(), "emo");
    EXPECT_FALSE((*tags_table)[1_idx].OptIn());
    EXPECT_EQ((*tags_table)[1_idx].Count(), 286);

    EXPECT_EQ((*tags_table)[2_idx].Name(), "neg");
    EXPECT_FALSE((*tags_table)[2_idx].OptIn());
    EXPECT_EQ((*tags_table)[2_idx].Count(), 56);

    EXPECT_EQ((*tags_table)[3_idx].Name(), "neut");
    EXPECT_FALSE((*tags_table)[3_idx].OptIn());
    EXPECT_EQ((*tags_table)[3_idx].Count(), 2014);

    EXPECT_EQ((*tags_table)[4_idx].Name(), "nsfw");
    EXPECT_TRUE((*tags_table)[4_idx].OptIn());
    EXPECT_EQ((*tags_table)[4_idx].Count(), 8);

    EXPECT_EQ((*tags_table)[5_idx].Name(), "obj");
    EXPECT_FALSE((*tags_table)[5_idx].OptIn());
    EXPECT_EQ((*tags_table)[5_idx].Count(), 5082);

    EXPECT_EQ((*tags_table)[6_idx].Name(), "pos");
    EXPECT_FALSE((*tags_table)[6_idx].OptIn());
    EXPECT_EQ((*tags_table)[6_idx].Count(), 86);

    consumed_size += detail::Align(tags_table->Size()).GetUnderlying();

    // WordData
    const auto* word_data = detail::WordData::GetWordData(data.subspan(consumed_size));
    EXPECT_TRUE(word_data->IsValid());
    EXPECT_EQ(word_data->MagicNum(), detail::WordData::kMagicNum);
    EXPECT_EQ(word_data->Count(), kTestWordCount);

    auto offset = (*index_table)[0_idx];
    EXPECT_EQ(word_data->At(offset).Lowercase(), "as");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "AS");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "As");
    EXPECT_EQ(word_data->At(offset).Size(), 18);

    offset = (*index_table)[1_idx];
    EXPECT_EQ(word_data->At(offset).Lowercase(), "by");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "BY");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "By");
    EXPECT_EQ(word_data->At(offset).Size(), 18);

    offset = (*index_table)[2_idx];
    EXPECT_EQ(word_data->At(offset).Lowercase(), "in");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "IN");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "In");
    EXPECT_EQ(word_data->At(offset).Size(), 18);

    auto middle_index = index_table->Count() / 2;
    offset = (*index_table)[middle_index - 1];
    // last word in the first half of the dictionary is the longest word
    EXPECT_EQ(word_data->At(offset).Lowercase(), "uncharacteristically");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "UNCHARACTERISTICALLY");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "Uncharacteristically");
    EXPECT_EQ(word_data->At(offset).Size(), 72);

    offset = (*index_table)[middle_index];
    // test data contains two identical dictionaries for different languages
    EXPECT_EQ(word_data->At(offset).Lowercase(), "as");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "AS");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "As");
    EXPECT_EQ(word_data->At(offset).Size(), 18);
}

void TestDictionary(std::span<const std::byte> data) {
    BinaryDictionary dictionary(data);
    EXPECT_EQ(dictionary.Kind(), "adverb");
    EXPECT_EQ(dictionary.Version(), "1.0.0");
    EXPECT_EQ(dictionary.Count(), kTestWordCount);

    EXPECT_EQ(dictionary.Languages(), (LanguageCodeSet{"en"_lang_view, "fr"_lang_view}));
    EXPECT_EQ(dictionary["en"_lang_view].Code(), "en");
    EXPECT_EQ(dictionary["en"_lang_view].Count(), kTestWordCount / 2);
    EXPECT_EQ(dictionary["fr"_lang_view].Code(), "fr");
    EXPECT_EQ(dictionary["fr"_lang_view].Count(), kTestWordCount / 2);
    EXPECT_THROW(dictionary["de"_lang_view], std::runtime_error);

    EXPECT_EQ(
        dictionary.Tags(),
        (TagSet{
            "det"_tag_view,
            "emo"_tag_view,
            "neg"_tag_view,
            "neut"_tag_view,
            "nsfw"_tag_view,
            "obj"_tag_view,
            "pos"_tag_view
        })
    );
    EXPECT_EQ(dictionary["det"_tag_view].Name(), "det");
    EXPECT_FALSE(dictionary["det"_tag_view].OptIn());
    EXPECT_EQ(dictionary["det"_tag_view].Count(), 6952);

    EXPECT_EQ(dictionary["emo"_tag_view].Name(), "emo");
    EXPECT_FALSE(dictionary["emo"_tag_view].OptIn());
    EXPECT_EQ(dictionary["emo"_tag_view].Count(), 286);

    EXPECT_EQ(dictionary["neg"_tag_view].Name(), "neg");
    EXPECT_FALSE(dictionary["neg"_tag_view].OptIn());
    EXPECT_EQ(dictionary["neg"_tag_view].Count(), 56);

    EXPECT_EQ(dictionary["neut"_tag_view].Name(), "neut");
    EXPECT_FALSE(dictionary["neut"_tag_view].OptIn());
    EXPECT_EQ(dictionary["neut"_tag_view].Count(), 2014);

    EXPECT_EQ(dictionary["nsfw"_tag_view].Name(), "nsfw");
    EXPECT_TRUE(dictionary["nsfw"_tag_view].OptIn());
    EXPECT_EQ(dictionary["nsfw"_tag_view].Count(), 8);

    EXPECT_EQ(dictionary["obj"_tag_view].Name(), "obj");
    EXPECT_FALSE(dictionary["obj"_tag_view].OptIn());
    EXPECT_EQ(dictionary["obj"_tag_view].Count(), 5082);

    EXPECT_EQ(dictionary["pos"_tag_view].Name(), "pos");
    EXPECT_FALSE(dictionary["pos"_tag_view].OptIn());
    EXPECT_EQ(dictionary["pos"_tag_view].Count(), 86);
    EXPECT_THROW(dictionary["foo"_tag_view], std::runtime_error);

    EXPECT_EQ(dictionary[0_idx].Lowercase(), "as");
    EXPECT_EQ(dictionary[0_idx].Uppercase(), "AS");
    EXPECT_EQ(dictionary[0_idx].Titlecase(), "As");
    EXPECT_EQ(dictionary[0_idx].Size(), 18);

    EXPECT_EQ(dictionary[1_idx].Lowercase(), "by");
    EXPECT_EQ(dictionary[1_idx].Uppercase(), "BY");
    EXPECT_EQ(dictionary[1_idx].Titlecase(), "By");
    EXPECT_EQ(dictionary[1_idx].Size(), 18);

    EXPECT_EQ(dictionary[2_idx].Lowercase(), "in");
    EXPECT_EQ(dictionary[2_idx].Uppercase(), "IN");
    EXPECT_EQ(dictionary[2_idx].Titlecase(), "In");
    EXPECT_EQ(dictionary[2_idx].Size(), 18);

    auto middle_index = kTestWordCount / 2;

    EXPECT_EQ(dictionary[middle_index - 1].Lowercase(), "uncharacteristically");
    EXPECT_EQ(dictionary[middle_index - 1].Uppercase(), "UNCHARACTERISTICALLY");
    EXPECT_EQ(dictionary[middle_index - 1].Titlecase(), "Uncharacteristically");
    EXPECT_EQ(dictionary[middle_index - 1].Size(), 72);

    EXPECT_EQ(dictionary[middle_index].Lowercase(), "as");
    EXPECT_EQ(dictionary[middle_index].Uppercase(), "AS");
    EXPECT_EQ(dictionary[middle_index].Titlecase(), "As");
    EXPECT_EQ(dictionary[middle_index].Size(), 18);
}

UTEST(BinaryDictionary, InMemoryTestData) {
    TestData(test::kDictionaryTestData);
}

UTEST(BinaryDictionary, InMemoryTestDictionary) {
    TestDictionary(test::kDictionaryTestData);
}

UTEST(BinaryDictionary, FileTestData) {
    utils::MemoryMappedFile file(test::kTestDictionaryFile);
    TestData(file.data());
}

UTEST(BinaryDictionary, FileTestDictionary) {
    utils::MemoryMappedFile file(test::kTestDictionaryFile);
    TestDictionary(file.data());
}

void TestFilteredBySize(std::span<const std::byte> data) {
    BinaryDictionary dictionary(data);
    auto filtered_dictionary = dictionary.Filter("adverb:<10"_selector);
    EXPECT_FALSE(filtered_dictionary->empty());
    EXPECT_EQ(filtered_dictionary->size(), 3340);
}

UTEST(BinaryDictionary, InMemoryTestFilteredBySize) {
    TestFilteredBySize(test::kDictionaryTestData);
}

UTEST(BinaryDictionary, FileTestFilteredBySize) {
    utils::MemoryMappedFile file(test::kTestDictionaryFile);
    TestFilteredBySize(file.data());
}

void TestFilteredByLangSize(std::span<const std::byte> data) {
    BinaryDictionary dictionary(data);
    auto filtered_dictionary = dictionary.Filter("adverb@en:<10"_selector);
    EXPECT_FALSE(filtered_dictionary->empty());
    EXPECT_EQ(filtered_dictionary->size(), 1670);
}

UTEST(BinaryDictionary, InMemoryTestFilteredByLangSize) {
    TestFilteredByLangSize(test::kDictionaryTestData);
}

UTEST(BinaryDictionary, FileTestFilteredByLangSize) {
    utils::MemoryMappedFile file(test::kTestDictionaryFile);
    TestFilteredByLangSize(file.data());
}

void TestFilteredByTags(std::span<const std::byte> data) {
    BinaryDictionary dictionary(data);
    auto filtered_dictionary = dictionary.Filter("adverb:+det-nsfw"_selector);
    EXPECT_FALSE(filtered_dictionary->empty());
    EXPECT_EQ(filtered_dictionary->size(), 6944);
}

UTEST(BinaryDictionary, InMemoryTestFilteredByTags) {
    TestFilteredByTags(test::kDictionaryTestData);
}

UTEST(BinaryDictionary, FileTestFilteredByTags) {
    utils::MemoryMappedFile file(test::kTestDictionaryFile);
    TestFilteredByTags(file.data());
}

void TestFilteredByLangTags(std::span<const std::byte> data) {
    BinaryDictionary dictionary(data);
    auto filtered_dictionary = dictionary.Filter("adverb@en:+det-nsfw"_selector);
    EXPECT_FALSE(filtered_dictionary->empty());
    EXPECT_EQ(filtered_dictionary->size(), 3472);
}

UTEST(BinaryDictionary, InMemoryTestFilteredByLangTags) {
    TestFilteredByLangTags(test::kDictionaryTestData);
}

UTEST(BinaryDictionary, FileTestFilteredByLangTags) {
    utils::MemoryMappedFile file(test::kTestDictionaryFile);
    TestFilteredByLangTags(file.data());
}

void TestFilteredByTagsSize(std::span<const std::byte> data) {
    BinaryDictionary dictionary(data);
    auto filtered_dictionary = dictionary.Filter("adverb:+det-nsfw<10"_selector);
    EXPECT_FALSE(filtered_dictionary->empty());
    EXPECT_EQ(filtered_dictionary->size(), 3200);
}

UTEST(BinaryDictionary, InMemoryTestFilteredByTagsSize) {
    TestFilteredByTagsSize(test::kDictionaryTestData);
}

UTEST(BinaryDictionary, FileTestFilteredByTagsSize) {
    utils::MemoryMappedFile file(test::kTestDictionaryFile);
    TestFilteredByTagsSize(file.data());
}

void TestFilteredByLangTagsSize(std::span<const std::byte> data) {
    BinaryDictionary dictionary(data);
    auto filtered_dictionary = dictionary.Filter("adverb@en:+det-nsfw<10"_selector);
    EXPECT_FALSE(filtered_dictionary->empty());
    EXPECT_EQ(filtered_dictionary->size(), 1600);
}

UTEST(BinaryDictionary, InMemoryTestFilteredByLangTagsSize) {
    TestFilteredByLangTagsSize(test::kDictionaryTestData);
}

UTEST(BinaryDictionary, FileTestFilteredByLangTagsSize) {
    utils::MemoryMappedFile file(test::kTestDictionaryFile);
    TestFilteredByLangTagsSize(file.data());
}

void TestFilteredToEmpty(std::span<const std::byte> data) {
    BinaryDictionary dictionary(data);
    auto filtered_dictionary = dictionary.Filter("adverb@en:+det +pos +nsfw==15"_selector);
    EXPECT_TRUE(filtered_dictionary->empty());
    EXPECT_EQ(filtered_dictionary->size(), 0);
}

UTEST(BinaryDictionary, InMemoryTestFilteredToEmpty) {
    TestFilteredToEmpty(test::kDictionaryTestData);
}

UTEST(BinaryDictionary, FileTestFilteredToEmpty) {
    utils::MemoryMappedFile file(test::kTestDictionaryFile);
    TestFilteredToEmpty(file.data());
}

}  // namespace slugkit::generator::binary
