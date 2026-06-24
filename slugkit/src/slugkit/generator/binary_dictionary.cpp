#include <slugkit/generator/binary_dictionary.hpp>

#include <slugkit/generator/exceptions.hpp>

#include <algorithm>

namespace slugkit::generator::binary {

namespace {
auto CheckPointer(const std::byte* base, std::string_view name, std::int64_t alignment = 4) -> void {
    if (base == nullptr) {
        throw DictionaryDataError(fmt::format("Invalid {}: base is nullptr", name));
    }
    if (reinterpret_cast<std::int64_t>(base) % alignment != 0) {
        throw DictionaryDataError(fmt::format("Invalid {}: base is not aligned to {}", name, alignment));
    }
}

// Validate an offset table up-front: every entry must point within @c data, relative to
// @c payload_base, with at least @c min_payload bytes available (the fixed header of the
// struct the offset addresses). After this passes, the offsets are trusted, so later
// pointer arithmetic + reinterpret_cast over them (including the section's iterators) can
// never read outside the mapped region. Also bounds-checks the offset table itself, so a
// corrupt @c count cannot walk off the buffer.
auto ValidateOffsetTable(
    RawData data,
    const detail::OffsetType* offset_table,
    IndexType count,
    const std::byte* payload_base,
    std::size_t min_payload,
    std::string_view section
) -> void {
    const auto* data_begin = data.data();
    const auto* data_end = data_begin + data.size();
    const auto* offset_table_begin = reinterpret_cast<const std::byte*>(offset_table);
    const auto* offset_table_end = reinterpret_cast<const std::byte*>(offset_table + count.GetUnderlying());
    if (offset_table_begin < data_begin || offset_table_end > data_end) {
        throw DictionaryDataError(
            fmt::format("Invalid {}: offset table of {} entries exceeds data bounds", section, count.GetUnderlying())
        );
    }
    if (payload_base < data_begin || payload_base > data_end) {
        throw DictionaryDataError(fmt::format("Invalid {}: payload base out of bounds", section));
    }
    const auto available = static_cast<std::size_t>(data_end - payload_base);
    for (IndexType::UnderlyingType i = 0; i < count.GetUnderlying(); ++i) {
        const auto offset = offset_table[i].GetUnderlying();
        if (offset > available || available - offset < min_payload) {
            throw DictionaryDataError(fmt::format(
                "Invalid {}: entry {} offset {} out of bounds (available {}, need >= {})",
                section,
                i,
                offset,
                available,
                min_payload
            ));
        }
    }
}
}  // namespace

namespace detail {
//-----------------------------------------------------------------------------
// SparseIndex
//-----------------------------------------------------------------------------
auto SparseIndex::GetSparseIndex(RawData data) -> const SparseIndex* {
    if (data.size() < sizeof(SparseIndex)) {
        throw DictionaryDataError(fmt::format(
            "Invalid data: size {} is less than sparse index expected size {}", data.size(), sizeof(SparseIndex)
        ));
    }
    const auto* sparse_index = GetSparseIndex(data.data());
    if (data.size() < sparse_index->Size().GetUnderlying()) {
        throw DictionaryDataError(fmt::format(
            "Invalid data: size {} is less than sparse index expected size {}", data.size(), sparse_index->Size()
        ));
    }
    return sparse_index;
}

auto SparseIndex::GetSparseIndex(const std::byte* base) -> const SparseIndex* {
    CheckPointer(base, "sparse index");
    const auto* sparse_index = reinterpret_cast<const SparseIndex*>(base);
    return sparse_index;
}

//-----------------------------------------------------------------------------
// LengthIndexTable
//-----------------------------------------------------------------------------
auto LengthIndexTable::Filter(SizeLimit size_limit) const -> filter::IndexRangeSequence {
    // depending on size limit operation, we need to find and grab the appropriate number of length indexes
    // and return them as a sequence
    auto apex = std::find_if(begin(), end(), [size_limit](const auto& length_index) {
        return length_index.length == size_limit.value;
    });

    switch (size_limit.op) {
        case CompareOperator::kEq:
            if (apex == end()) {
                return {};
            }
            return {apex->range.Filter()};
        case CompareOperator::kNe:
            if (apex == end()) {
                // everything in range
                return {front().range.start_index, back().range.end_index};
            } else if (apex == begin()) {
                return {apex->range.end_index, back().range.end_index};
            } else {
                return {
                    filter::IndexRange{front().range.start_index, apex->range.start_index},
                    filter::IndexRange{apex->range.end_index, back().range.end_index}
                };
            }
        case CompareOperator::kGt:
            if (apex == end()) {
                return {};
            }
            return {apex->range.end_index, back().range.end_index};
        case CompareOperator::kGe:
            if (apex == end()) {
                return {};
            }
            return {apex->range.start_index, back().range.end_index};
        case CompareOperator::kLt:
            if (apex == begin()) {
                return {};
            }
            if (apex == end()) {
                return {front().range.start_index, back().range.end_index};
            }
            return {front().range.start_index, apex->range.start_index};
        case CompareOperator::kLe:
            if (apex == end()) {
                return {front().range.start_index, back().range.end_index};
            }
            return {front().range.start_index, apex->range.end_index};
        default:
            throw DictionaryFilterError(
                fmt::format("Invalid size limit operation: {}", static_cast<std::int64_t>(size_limit.op))
            );
    }
}

//-----------------------------------------------------------------------------
// Header
//-----------------------------------------------------------------------------
auto Header::GetHeader(RawData data) -> const Header* {
    if (data.size() < sizeof(Header)) {
        throw DictionaryDataError(fmt::format("Invalid data: size {} is less than {}", data.size(), sizeof(Header)));
    }
    const auto* header = GetHeader(data.data());
    if (header->Size().GetUnderlying() > data.size()) {
        throw DictionaryDataError(
            fmt::format("Invalid data: size {} is less than header expected size {}", data.size(), header->Size())
        );
    }
    return header;
}

auto Header::GetHeader(const std::byte* base) -> const Header* {
    CheckPointer(base, "header");
    const auto* header = reinterpret_cast<const Header*>(base);
    if (!header->IsValid()) {
        throw DictionaryDataError(fmt::format("Invalid header: magic num {} is not {}", header->MagicNum(), kMagicNum));
    }
    return header;
}

//-----------------------------------------------------------------------------
// IndexTable
//-----------------------------------------------------------------------------
auto IndexTable::GetIndexTable(RawData data) -> const IndexTable* {
    if (data.size() < sizeof(IndexTable)) {
        throw DictionaryDataError(fmt::format(
            "Invalid data: size {} is less than index table expected size {}", data.size(), sizeof(IndexTable)
        ));
    }
    const auto* index_table = GetIndexTable(data.data());
    if (data.size() < index_table->Size().GetUnderlying() + sizeof(OffsetType)) {
        throw DictionaryDataError(fmt::format(
            "Invalid data: size {} is less than index table expected size {}",
            data.size(),
            index_table->Size().GetUnderlying() + sizeof(OffsetType)
        ));
    }
    return index_table;
}

auto IndexTable::GetIndexTable(const std::byte* base) -> const IndexTable* {
    CheckPointer(base, "index table");
    const auto* index_table = reinterpret_cast<const IndexTable*>(base);
    if (!index_table->IsValid()) {
        throw DictionaryDataError(
            fmt::format("Invalid index table: magic num {} is not {}", index_table->MagicNum(), kMagicNum)
        );
    }
    return index_table;
}

//-----------------------------------------------------------------------------
// LanguageTable
//-----------------------------------------------------------------------------
auto LanguageTable::Languages() const noexcept -> LanguageCodeSet {
    LanguageCodeSet languages;
    for (const auto& language_info : *this) {
        languages.insert(language_info.Code());
    }
    return languages;
}

auto LanguageTable::Filter(std::optional<LanguageCodeView> language, std::optional<SizeLimit> size_limit) const
    -> filter::IndexRangeSequence {
    if (language) {
        return (*this)[*language].Filter(size_limit);
    }
    filter::IndexRangeSequence sequence;
    for (const auto& language_info : *this) {
        sequence = sequence + language_info.Filter(size_limit);
    }
    return sequence;
}

auto LanguageTable::operator[](LanguageCodeView language) const -> const LanguageInfo& {
    for (const auto& language_info : *this) {
        if (language_info.Code() == language) {
            return language_info;
        }
    }
    throw DictionaryFilterError(fmt::format("Language code `{}` not found", language));
}

auto LanguageTable::GetLanguageTable(RawData data) -> const LanguageTable* {
    if (data.size() < sizeof(LanguageTable)) {
        throw DictionaryDataError(fmt::format(
            "Invalid data: size {} is less than language table expected size {}", data.size(), sizeof(LanguageTable)
        ));
    }
    const auto* language_table = GetLanguageTable(data.data());
    if (data.size() < language_table->Size().GetUnderlying() + sizeof(OffsetType)) {
        throw DictionaryDataError(fmt::format(
            "Invalid data: size {} is less than language table expected size {}",
            data.size(),
            language_table->Size().GetUnderlying() + sizeof(OffsetType)
        ));
    }
    ValidateOffsetTable(
        data,
        language_table->GetOffsetTableBase(),
        language_table->Count(),
        language_table->GetLanguageInfoTableBase(),
        sizeof(LanguageInfo),
        "language table"
    );
    return language_table;
}

auto LanguageTable::GetLanguageTable(const std::byte* base) -> const LanguageTable* {
    CheckPointer(base, "language table");
    const auto* language_table = reinterpret_cast<const LanguageTable*>(base);
    if (!language_table->IsValid()) {
        throw DictionaryDataError(
            fmt::format("Invalid language table: magic num {} is not {}", language_table->MagicNum(), kMagicNum)
        );
    }
    return language_table;
}

//-----------------------------------------------------------------------------
// TagsTable
//-----------------------------------------------------------------------------
auto TagsTable::Tags() const noexcept -> TagSet {
    TagSet tags;
    for (const auto& tag_entry : *this) {
        tags.insert(tag_entry.Name());
    }
    return tags;
}

// Filter is:
// Intersection of include tags and difference of exclude tags from include tags
// Empty tag list - all tags
auto TagsTable::Filter(
    const filter::IndexRangeSequence& index_range_sequence,
    const TagSet& include_tags,
    const TagSet& exclude_tags
) const -> filter::IndexSequence {
    if (include_tags.empty() && exclude_tags.empty()) {
        return {index_range_sequence};
    }
    filter::IndexSequence indices{index_range_sequence};
    for (auto in_tag : include_tags) {
        auto tag_entry = FindTagEntry(in_tag);
        if (!tag_entry) {
            throw DictionaryFilterError(fmt::format("Tag `{}` not found", in_tag));
        }
        indices = indices * tag_entry->Filter();
    }
    for (const auto& exclude_tag : exclude_tags) {
        auto tag_entry = FindTagEntry(exclude_tag);
        if (!tag_entry) {
            throw DictionaryFilterError(fmt::format("Tag `{}` not found", exclude_tag));
        }
        indices = indices - tag_entry->Filter();
    }
    // TODO opt-in tags
    return indices;
}

auto TagsTable::GetTagFilter(TagView tag) const -> filter::IndexSetView {
    auto tag_entry = FindTagEntry(tag);
    if (!tag_entry) {
        throw DictionaryFilterError(fmt::format("Tag `{}` not found", tag));
    }
    return tag_entry->Filter();
}

auto TagsTable::FindTagEntry(TagView tag) const -> const TagEntry* {
    auto it = std::lower_bound(begin(), end(), tag, [](const TagEntry& entry, TagView search_tag) {
        return entry.Name() < search_tag;
    });
    if (it != end() && it->Name() == tag) {
        return &(*it);
    }
    return nullptr;
}

auto TagsTable::operator[](TagView tag) const -> const TagEntry& {
    auto tag_entry = FindTagEntry(tag);
    if (tag_entry) {
        return *tag_entry;
    }
    throw DictionaryFilterError(fmt::format("Tag `{}` not found", tag));
}

auto TagsTable::operator[](Tag tag) const -> const TagEntry& {
    auto tag_view = TagView(tag.GetUnderlying());
    return (*this)[tag_view];
}

auto TagsTable::GetTagsTable(RawData data) -> const TagsTable* {
    if (data.size() < sizeof(TagsTable)) {
        throw DictionaryDataError(fmt::format(
            "Invalid data: size {} is less than tags table expected size {}", data.size(), sizeof(TagsTable)
        ));
    }
    const auto* tags_table = GetTagsTable(data.data());
    ValidateOffsetTable(
        data,
        tags_table->GetOffsetTableBase(),
        tags_table->Count(),
        tags_table->GetTagDataBase(),
        sizeof(TagEntry),
        "tags table"
    );
    return tags_table;
}

auto TagsTable::GetTagsTable(const std::byte* base) -> const TagsTable* {
    CheckPointer(base, "tags table");
    const auto* tags_table = reinterpret_cast<const TagsTable*>(base);
    if (!tags_table->IsValid()) {
        throw DictionaryDataError(
            fmt::format("Invalid tags table: magic num {} is not {}", tags_table->MagicNum(), kMagicNum)
        );
    }
    return tags_table;
}

//-----------------------------------------------------------------------------
// WordData
//-----------------------------------------------------------------------------
auto WordData::GetWordData(RawData data) -> const WordData* {
    if (data.size() < sizeof(WordData)) {
        throw DictionaryDataError(
            fmt::format("Invalid data: size {} is less than word data expected size {}", data.size(), sizeof(WordData))
        );
    }
    const auto* word_data = GetWordData(data.data());
    if (data.size() < word_data->Size().GetUnderlying()) {
        throw DictionaryDataError(fmt::format(
            "Invalid data: size {} is less than word data expected size {}",
            data.size(),
            word_data->Size().GetUnderlying() + sizeof(OffsetType)
        ));
    }
    return word_data;
}

auto WordData::GetWordData(const std::byte* base) -> const WordData* {
    CheckPointer(base, "word data");
    const auto* word_data = reinterpret_cast<const WordData*>(base);
    if (!word_data->IsValid()) {
        throw DictionaryDataError(
            fmt::format("Invalid word data: magic num {} is not {}", word_data->MagicNum(), kMagicNum)
        );
    }
    return word_data;
}

}  // namespace detail

//-----------------------------------------------------------------------------
// LanguageInfo
//-----------------------------------------------------------------------------
auto LanguageInfo::GetLanguageInfo(RawData data) -> const LanguageInfo* {
    if (data.size() < sizeof(LanguageInfo)) {
        throw DictionaryDataError(fmt::format(
            "Invalid data: size {} is less than language info expected size {}", data.size(), sizeof(LanguageInfo)
        ));
    }
    const auto* language_info = GetLanguageInfo(data.data());
    if (data.size() < language_info->Size().GetUnderlying() + sizeof(detail::OffsetType)) {
        throw DictionaryDataError(fmt::format(
            "Invalid data: size {} is less than language info expected size {}",
            data.size(),
            language_info->Size().GetUnderlying() + sizeof(detail::OffsetType)
        ));
    }
    return language_info;
}

auto LanguageInfo::GetLanguageInfo(const std::byte* base) -> const LanguageInfo* {
    CheckPointer(base, "language info");
    const auto* language_info = reinterpret_cast<const LanguageInfo*>(base);
    if (!language_info->IsValid()) {
        throw DictionaryDataError(fmt::format(
            "Invalid language info: magic num {} is not {}",
            language_info->length_index_table_.MagicNum(),
            detail::LengthIndexTable::kMagicNum
        ));
    }
    return language_info;
}

//-----------------------------------------------------------------------------
// TagEntry
//-----------------------------------------------------------------------------
auto TagEntry::GetTagEntry(RawData data) -> const TagEntry* {
    if (data.size() < sizeof(TagEntry)) {
        throw DictionaryDataError(
            fmt::format("Invalid data: size {} is less than tag entry expected size {}", data.size(), sizeof(TagEntry))
        );
    }
    return GetTagEntry(data.data());
}

auto TagEntry::GetTagEntry(const std::byte* base) -> const TagEntry* {
    CheckPointer(base, "tag entry");
    const auto* tag_entry = reinterpret_cast<const TagEntry*>(base);
    if (!tag_entry->IsValid()) {
        throw DictionaryDataError(
            fmt::format("Invalid tag entry: magic num {} is not {}", tag_entry->MagicNum(), kMagicNum)
        );
    }
    return tag_entry;
}

//-----------------------------------------------------------------------------
// FilteredDictionary
//-----------------------------------------------------------------------------
auto FilteredDictionary::operator[](IndexType index) const -> const WordEntry& {
    auto offset = (*index_table_)[index];
    return word_data_->At(offset);
}

//-----------------------------------------------------------------------------
// BinaryDictionary
//-----------------------------------------------------------------------------
BinaryDictionary::BinaryDictionary(RawData data)
    : data_(data) {
    header_ = detail::Header::GetHeader(data_);
    auto consumed_size = detail::Align(header_->Size()).GetUnderlying();

    index_table_ = detail::IndexTable::GetIndexTable(data_.subspan(consumed_size));
    consumed_size += detail::Align(index_table_->Size()).GetUnderlying();

    language_table_ = detail::LanguageTable::GetLanguageTable(data_.subspan(consumed_size));
    consumed_size += detail::Align(language_table_->Size()).GetUnderlying();

    tags_table_ = detail::TagsTable::GetTagsTable(data_.subspan(consumed_size));
    consumed_size += detail::Align(tags_table_->Size()).GetUnderlying();

    word_data_ = detail::WordData::GetWordData(data_.subspan(consumed_size));
}

auto BinaryDictionary::operator[](LanguageCodeView language) const -> const LanguageInfo& {
    return (*language_table_)[language];
}

auto BinaryDictionary::operator[](Tag tag) const -> const TagEntry& {
    return (*tags_table_)[tag];
}

auto BinaryDictionary::operator[](TagView tag) const -> const TagEntry& {
    return (*tags_table_)[tag];
}

auto BinaryDictionary::operator[](IndexType index) const -> const WordEntry& {
    auto offset = (*index_table_)[index];
    return word_data_->At(offset);
}

auto BinaryDictionary::Filter(const Selector& selector) const -> FilteredDictionaryPtr {
    auto index_range_sequence = language_table_->Filter(selector.language, selector.size_limit);
    auto indices = tags_table_->Filter(index_range_sequence, selector.include_tags, selector.exclude_tags);
    // TODO opt-in tags
    return std::make_shared<FilteredDictionary>(indices, index_table_, word_data_);
}

}  // namespace slugkit::generator::binary
