#include <slugkit/generator/binary_dictionary.hpp>

#include "test_dictionary.hpp"

#include <slugkit/test_utils/data_config.hpp>

#include <slugkit/utils/memory_mapped_file.hpp>

#include <userver/utest/utest.hpp>

namespace slugkit::generator::binary {

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
    const auto* header = Header::GetHeader(kHeaderData);
    EXPECT_TRUE(header->IsValid());
    EXPECT_EQ(header->MagicNum(), "SLUGDICT");
    EXPECT_EQ(header->Size(), SizeType(54));
    EXPECT_EQ(header->Kind(), "noun");
    EXPECT_EQ(header->Version(), "0.0.1");
    EXPECT_EQ(header->Description(), "dictionary definition");
}

UTEST(BinaryDictionary, WordEntry) {
    const auto* word_entry = WordEntry::GetWordEntry(kWordEntryData.subspan(WordData::kMagicNum.size(), 24));
    EXPECT_EQ(word_entry->Lowercase(), "noun");
    EXPECT_EQ(word_entry->Uppercase(), "NOUN");
    EXPECT_EQ(word_entry->Titlecase(), "Noun");
    EXPECT_EQ(word_entry->Size(), 24);
}

UTEST(BinaryDictionary, DISABLED_WordData) {
    const auto* word_data = WordData::GetWordData(kWordEntryData);
    const auto offset = OffsetType(0);
    EXPECT_EQ(word_data->At(offset).Lowercase(), "noun");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "NOUN");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "Noun");
    EXPECT_EQ(word_data->At(offset).Size(), 24);
}

void TestData(std::span<const std::byte> data) {
    // Header
    const auto* header = Header::GetHeader(data);
    EXPECT_TRUE(header->IsValid());
    EXPECT_EQ(header->MagicNum(), Header::kMagicNum);
    EXPECT_EQ(header->Size(), 58);
    EXPECT_EQ(header->Kind(), "adverb");
    EXPECT_EQ(header->Version(), "1.0.0");
    EXPECT_EQ(header->Description(), "A dictionary of adverbs");

    auto consumed_size = Align(header->Size()).GetUnderlying();

    // IndexTable
    const auto* index_table = IndexTable::GetIndexTable(data.subspan(consumed_size));
    EXPECT_TRUE(index_table->IsValid());
    EXPECT_EQ(index_table->MagicNum(), IndexTable::kMagicNum);
    EXPECT_EQ(index_table->Count(), kTestWordCount);
    EXPECT_EQ((*index_table)[IndexType(0)], 0);
    EXPECT_EQ((*index_table)[IndexType(1)], 18);
    EXPECT_EQ((*index_table)[IndexType(2)], 36);
    EXPECT_EQ((*index_table)[IndexType(3)], 54);

    consumed_size += Align(index_table->Size()).GetUnderlying();

    // LanguageTable
    const auto* language_table = LanguageTable::GetLanguageTable(data.subspan(consumed_size));
    EXPECT_TRUE(language_table->IsValid());
    EXPECT_EQ(language_table->MagicNum(), LanguageTable::kMagicNum);
    EXPECT_EQ(language_table->Count(), 2);
    EXPECT_EQ((*language_table)[IndexType(0)].Code(), "en");
    EXPECT_EQ((*language_table)[IndexType(0)].Count(), kTestWordCount / 2);
    EXPECT_EQ((*language_table)[IndexType(1)].Code(), "fr");
    EXPECT_EQ((*language_table)[IndexType(1)].Count(), kTestWordCount / 2);

    consumed_size += Align(language_table->Size()).GetUnderlying();

    // TagsTable
    const auto* tags_table = TagsTable::GetTagsTable(data.subspan(consumed_size));
    EXPECT_TRUE(tags_table->IsValid());
    EXPECT_EQ(tags_table->MagicNum(), TagsTable::kMagicNum);
    EXPECT_EQ(tags_table->Count(), 7);

    EXPECT_EQ((*tags_table)[IndexType(0)].Name(), "det");
    EXPECT_FALSE((*tags_table)[IndexType(0)].OptIn());
    EXPECT_EQ((*tags_table)[IndexType(0)].Count(), 6952);

    EXPECT_EQ((*tags_table)[IndexType(1)].Name(), "emo");
    EXPECT_FALSE((*tags_table)[IndexType(1)].OptIn());
    EXPECT_EQ((*tags_table)[IndexType(1)].Count(), 286);

    EXPECT_EQ((*tags_table)[IndexType(2)].Name(), "neg");
    EXPECT_FALSE((*tags_table)[IndexType(2)].OptIn());
    EXPECT_EQ((*tags_table)[IndexType(2)].Count(), 56);

    EXPECT_EQ((*tags_table)[IndexType(3)].Name(), "neut");
    EXPECT_FALSE((*tags_table)[IndexType(3)].OptIn());
    EXPECT_EQ((*tags_table)[IndexType(3)].Count(), 2014);

    EXPECT_EQ((*tags_table)[IndexType(4)].Name(), "nsfw");
    EXPECT_TRUE((*tags_table)[IndexType(4)].OptIn());
    EXPECT_EQ((*tags_table)[IndexType(4)].Count(), 8);

    EXPECT_EQ((*tags_table)[IndexType(5)].Name(), "obj");
    EXPECT_FALSE((*tags_table)[IndexType(5)].OptIn());
    EXPECT_EQ((*tags_table)[IndexType(5)].Count(), 5082);

    EXPECT_EQ((*tags_table)[IndexType(6)].Name(), "pos");
    EXPECT_FALSE((*tags_table)[IndexType(6)].OptIn());
    EXPECT_EQ((*tags_table)[IndexType(6)].Count(), 86);

    consumed_size += Align(tags_table->Size()).GetUnderlying();

    // WordData
    const auto* word_data = WordData::GetWordData(data.subspan(consumed_size));
    EXPECT_TRUE(word_data->IsValid());
    EXPECT_EQ(word_data->MagicNum(), WordData::kMagicNum);

    auto offset = (*index_table)[IndexType(0)];
    EXPECT_EQ(word_data->At(offset).Lowercase(), "as");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "AS");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "As");
    EXPECT_EQ(word_data->At(offset).Size(), 18);

    offset = (*index_table)[IndexType(1)];
    EXPECT_EQ(word_data->At(offset).Lowercase(), "by");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "BY");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "By");
    EXPECT_EQ(word_data->At(offset).Size(), 18);

    offset = (*index_table)[IndexType(2)];
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

UTEST(BinaryDictionary, InMemoryTestData) {
    TestData(test::kDictionaryTestData);
}

UTEST(BinaryDictionary, FileTestData) {
    utils::MemoryMappedFile file(test::kTestDictionaryFile);
    TestData(file.data());
}

}  // namespace slugkit::generator::binary
