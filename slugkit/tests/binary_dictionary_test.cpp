#include <slugkit/generator/binary_dictionary.hpp>

#include <slugkit/test_utils/test_dictionary.hpp>

#include <slugkit/test_utils/data_config.hpp>

#include <slugkit/utils/memory_mapped_file.hpp>

#include <slugkit/generator/exceptions.hpp>

#include <userver/utest/utest.hpp>

#include <vector>

namespace slugkit::generator::binary {

using namespace literals;
using namespace detail::literals;
using namespace generator::literals;

namespace {

constexpr IndexType kTestWordCount{7238};

constexpr char kHeaderRawData[] =
    "SLUGDICT"
    "\x02\0\0\0"             // binary format version, 2 (little-endian 4 bytes)
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

// A complete word data section: magic + size + count + one 24-byte word entry. (Unlike
// kWordEntryRawData above, this includes the WordData header fields so WordData::At works.)
constexpr char kWordDataRawData[] =
    "WORDS==="
    "\x18\0\0\0"  // size: 24-byte word-data region (little-endian 4 bytes)
    "\x01\0\0\0"  // count: 1 word (little-endian 4 bytes)
    "\0\0"        // lowercase markup, start 0 (little-endian 2 bytes)
    "\x04\0"      // lowercase markup, length 4 (little-endian 2 bytes)
    "\x04\0"      // uppercase markup, start 4 (little-endian 2 bytes)
    "\x04\0"      // uppercase markup, length 4 (little-endian 2 bytes)
    "\x08\0"      // titlecase markup, start 8 (little-endian 2 bytes)
    "\x04\0"      // titlecase markup, length 4 (little-endian 2 bytes)
    "noun"        // lowercase
    "NOUN"        // uppercase
    "Noun"        // titlecase
    ;

const std::span<const std::byte> kWordDataData =
    std::span<const std::byte>(reinterpret_cast<const std::byte*>(kWordDataRawData), sizeof(kWordDataRawData));

// Copy a dictionary blob into a mutable buffer so individual bytes can be corrupted.
auto Mutate(std::span<const std::byte> src) -> std::vector<std::byte> {
    return std::vector<std::byte>(src.begin(), src.end());
}

// Byte offset of the language table within a (valid) dictionary blob.
auto LanguageTableOffset(std::span<const std::byte> data) -> std::size_t {
    const auto* header = detail::Header::GetHeader(data);
    auto consumed = detail::Align(header->Size()).GetUnderlying();
    const auto* index_table = detail::IndexTable::GetIndexTable(data.subspan(consumed));
    consumed += detail::Align(index_table->Size()).GetUnderlying();
    return consumed;
}

}  // namespace

UTEST(BinaryDictionary, Header) {
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

UTEST(BinaryDictionary, WordData) {
    const auto* word_data = detail::WordData::GetWordData(kWordDataData);
    const auto offset = 0_off;
    EXPECT_EQ(word_data->At(offset).Lowercase(), "noun");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "NOUN");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "Noun");
    EXPECT_EQ(word_data->At(offset).Size(), 24);
}

// Constructing a dictionary from corrupt bytes must be rejected with DictionaryDataError
// rather than read out of bounds. Each case starts from a valid dictionary and corrupts a
// single field to trip a specific guard.
UTEST(BinaryDictionary, RejectsCorruptData) {
    const auto valid = test::kDictionaryTestData;
    // Baseline: the unmodified dictionary constructs (and fully validates) without throwing.
    ASSERT_NO_THROW((BinaryDictionary{RawData{valid}}));

    // 1. Bad header magic -> magic check in the header factory.
    {
        auto buf = Mutate(valid);
        buf[0] = std::byte{'X'};
        EXPECT_THROW((BinaryDictionary{RawData{buf}}), DictionaryDataError);
    }

    // 2. Truncated data -> bounds check in the header factory.
    EXPECT_THROW((BinaryDictionary{valid.subspan(0, detail::Header::kMagicNum.size())}), DictionaryDataError);

    // 3. A language offset-table entry pointing out of bounds -> ValidateOffsetTable.
    {
        auto buf = Mutate(valid);
        const auto lang = LanguageTableOffset(buf);
        const auto offset_table = lang + detail::LanguageTable::kMagicNum.size() + sizeof(SizeType) + sizeof(IndexType);
        for (std::size_t i = 0; i < sizeof(detail::OffsetType); ++i) {
            buf[offset_table + i] = std::byte{0xFF};
        }
        EXPECT_THROW((BinaryDictionary{RawData{buf}}), DictionaryDataError);
    }

    // 4. A corrupt language count, so the offset table no longer fits -> ValidateOffsetTable.
    {
        auto buf = Mutate(valid);
        const auto lang = LanguageTableOffset(buf);
        const auto count_offset = lang + detail::LanguageTable::kMagicNum.size() + sizeof(SizeType);
        for (std::size_t i = 0; i < sizeof(IndexType); ++i) {
            buf[count_offset + i] = std::byte{0xFF};
        }
        EXPECT_THROW((BinaryDictionary{RawData{buf}}), DictionaryDataError);
    }
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
    // The logical word order is lexicographic: "aback" (27 bytes), "abaft" (27),
    // "abaxially" (39), ... so the offsets accumulate the variable entry sizes.
    EXPECT_EQ((*index_table)[IndexType(0)], 0);
    EXPECT_EQ((*index_table)[IndexType(1)], 27);
    EXPECT_EQ((*index_table)[IndexType(2)], 54);
    EXPECT_EQ((*index_table)[IndexType(3)], 93);

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

    // Words are now in lexicographic order, so the first words are the alphabetically
    // smallest ones rather than the shortest.
    auto offset = (*index_table)[0_idx];
    EXPECT_EQ(word_data->At(offset).Lowercase(), "aback");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "ABACK");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "Aback");
    EXPECT_EQ(word_data->At(offset).Size(), 27);

    offset = (*index_table)[1_idx];
    EXPECT_EQ(word_data->At(offset).Lowercase(), "abaft");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "ABAFT");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "Abaft");
    EXPECT_EQ(word_data->At(offset).Size(), 27);

    offset = (*index_table)[2_idx];
    EXPECT_EQ(word_data->At(offset).Lowercase(), "abaxially");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "ABAXIALLY");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "Abaxially");
    EXPECT_EQ(word_data->At(offset).Size(), 39);

    auto middle_index = index_table->Count() / 2;
    offset = (*index_table)[middle_index - 1];
    // last word in the first half of the dictionary is the alphabetically last word
    EXPECT_EQ(word_data->At(offset).Lowercase(), "zigzag");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "ZIGZAG");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "Zigzag");
    EXPECT_EQ(word_data->At(offset).Size(), 30);

    offset = (*index_table)[middle_index];
    // test data contains two identical dictionaries for different languages
    EXPECT_EQ(word_data->At(offset).Lowercase(), "aback");
    EXPECT_EQ(word_data->At(offset).Uppercase(), "ABACK");
    EXPECT_EQ(word_data->At(offset).Titlecase(), "Aback");
    EXPECT_EQ(word_data->At(offset).Size(), 27);
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

    EXPECT_EQ(dictionary[0_idx].Lowercase(), "aback");
    EXPECT_EQ(dictionary[0_idx].Uppercase(), "ABACK");
    EXPECT_EQ(dictionary[0_idx].Titlecase(), "Aback");
    EXPECT_EQ(dictionary[0_idx].Size(), 27);

    EXPECT_EQ(dictionary[1_idx].Lowercase(), "abaft");
    EXPECT_EQ(dictionary[1_idx].Uppercase(), "ABAFT");
    EXPECT_EQ(dictionary[1_idx].Titlecase(), "Abaft");
    EXPECT_EQ(dictionary[1_idx].Size(), 27);

    EXPECT_EQ(dictionary[2_idx].Lowercase(), "abaxially");
    EXPECT_EQ(dictionary[2_idx].Uppercase(), "ABAXIALLY");
    EXPECT_EQ(dictionary[2_idx].Titlecase(), "Abaxially");
    EXPECT_EQ(dictionary[2_idx].Size(), 39);

    auto middle_index = kTestWordCount / 2;

    EXPECT_EQ(dictionary[middle_index - 1].Lowercase(), "zigzag");
    EXPECT_EQ(dictionary[middle_index - 1].Uppercase(), "ZIGZAG");
    EXPECT_EQ(dictionary[middle_index - 1].Titlecase(), "Zigzag");
    EXPECT_EQ(dictionary[middle_index - 1].Size(), 30);

    EXPECT_EQ(dictionary[middle_index].Lowercase(), "aback");
    EXPECT_EQ(dictionary[middle_index].Uppercase(), "ABACK");
    EXPECT_EQ(dictionary[middle_index].Titlecase(), "Aback");
    EXPECT_EQ(dictionary[middle_index].Size(), 27);
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
