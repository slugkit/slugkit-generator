#include <slugkit/generator/binary_dictionary.hpp>

#include <slugkit/generator/exceptions.hpp>

#include <slugkit/utils/text.hpp>

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

// Throw unless the region [ptr, ptr + size) lies fully within @c data. Used by the
// recursive validation to bound every variable-length sub-structure (strings, sparse
// indexes, length-index tables) against the backing buffer.
auto WithinData(RawData data, const void* ptr, std::size_t size, std::string_view what) -> void {
    const auto* begin = data.data();
    const auto* end = begin + data.size();
    const auto* p = static_cast<const std::byte*>(ptr);
    if (p < begin || p > end || size > static_cast<std::size_t>(end - p)) {
        throw DictionaryDataError(
            fmt::format("Invalid {}: a {}-byte region lies outside the dictionary data", what, size)
        );
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
auto LengthIndexTable::Filter(SizeLimit size_limit) const -> filter::IndexSet {
    // The logical word order is lexicographic, so words of a given length are not contiguous.
    // Collect the lex positions of every length that satisfies the size limit and merge them
    // into a single ascending (lexicographic) set. Each per-length sparse index is already
    // ascending, and the buckets are disjoint, so a sort suffices to merge them.
    std::vector<IndexType> result;
    for (const auto& length_index : *this) {
        if (size_limit.Matches(length_index.Length().GetUnderlying())) {
            const auto& sparse = length_index.Index();
            result.insert(result.end(), sparse.begin(), sparse.end());
        }
    }
    std::sort(result.begin(), result.end());
    return filter::IndexSet(std::move(result));
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
    -> filter::IndexSequence {
    if (language) {
        return (*this)[*language].Filter(size_limit);
    }
    // Combine across all languages. Without a length limit each language contributes its
    // contiguous range, so the union stays a range sequence. With a length limit each
    // contributes a sparse set, so the union is a single ascending (lex) set.
    if (!size_limit) {
        filter::IndexRangeSequence sequence;
        for (const auto& language_info : *this) {
            sequence = sequence + language_info.FullRange();
        }
        return sequence;
    }
    filter::IndexSet set;
    for (const auto& language_info : *this) {
        set = set + language_info.LengthFilter(*size_limit);
    }
    return set;
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
    const filter::IndexSequence& index_sequence,
    const TagSet& include_tags,
    const TagSet& exclude_tags
) const -> filter::IndexSequence {
    if (include_tags.empty() && exclude_tags.empty()) {
        return index_sequence;
    }
    filter::IndexSequence indices{index_sequence};
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
// Recursive validation
//-----------------------------------------------------------------------------
namespace detail {

auto SparseIndex::Validate(RawData data, IndexType word_count) const -> void {
    const auto count = static_cast<std::size_t>(count_.GetUnderlying());
    WithinData(data, this, sizeof(IndexType) * (count + 1), "sparse index");
    for (const auto& index : *this) {
        if (index >= word_count) {
            throw DictionaryDataError(
                fmt::format("Invalid sparse index: word index {} is >= word count {}", index, word_count)
            );
        }
    }
}

auto LengthIndexTable::Validate(RawData data, IndexType word_count) const -> void {
    if (!IsValid()) {
        throw DictionaryDataError(
            fmt::format("Invalid length index table: magic num {} is not {}", MagicNum(), kMagicNum)
        );
    }
    // Bound the fixed header (magic + count) before walking the variable-size entries.
    WithinData(data, this, sizeof(LengthIndexTable), "length index table");
    for (const auto& length_index : *this) {
        // Bound the entry's fixed prefix (length + the sparse-index count) before reading
        // the sparse index, which determines the entry's full size.
        WithinData(data, &length_index, sizeof(SizeType) + sizeof(IndexType), "length index entry");
        length_index.Index().Validate(data, word_count);
    }
}

auto LanguageTable::Validate(RawData data, IndexType word_count) const -> void {
    // Offsets were bounds-checked by ValidateOffsetTable, so iteration is safe.
    for (const auto& language_info : *this) {
        language_info.Validate(data, word_count);
    }
}

auto TagsTable::Validate(RawData data, IndexType word_count) const -> void {
    for (const auto& tag_entry : *this) {
        tag_entry.Validate(data, word_count);
    }
}

}  // namespace detail

auto LanguageInfo::Validate(RawData data, IndexType word_count) const -> void {
    WithinData(data, this, sizeof(LanguageInfo), "language info");
    bool terminated = false;
    for (std::size_t i = 0; i < kCodeSize; ++i) {
        if (code_[i] == '\0') {
            terminated = true;
            break;
        }
    }
    if (!terminated) {
        throw DictionaryDataError("Invalid language info: language code is not null-terminated");
    }
    if (!range_.IsValid() || range_.end_index > word_count) {
        throw DictionaryDataError(fmt::format(
            "Invalid language info: range [{}, {}) is not within [0, {}]",
            range_.start_index,
            range_.end_index,
            word_count
        ));
    }
    length_index_table_.Validate(data, word_count);
}

auto TagEntry::Validate(RawData data, IndexType word_count) const -> void {
    if (!IsValid()) {
        throw DictionaryDataError(fmt::format("Invalid tag entry: magic num {} is not {}", MagicNum(), kMagicNum));
    }
    WithinData(data, this, sizeof(TagEntry), "tag entry");
    const auto name = Name().GetUnderlying();
    WithinData(data, name.data(), name.size(), "tag name");
    const auto description = Description();
    WithinData(data, description.data(), description.size(), "tag description");
    // Bound the sparse-index header before dereferencing it (Index() reads its count).
    WithinData(data, GetIndexBase(), sizeof(IndexType), "tag sparse index");
    Index().Validate(data, word_count);
}

auto WordEntry::Validate(RawData data) const -> void {
    WithinData(data, this, sizeof(WordEntry), "word entry");
    const auto lowercase = Lowercase();
    WithinData(data, lowercase.data(), lowercase.size(), "word lowercase");
    const auto uppercase = Uppercase();
    WithinData(data, uppercase.data(), uppercase.size(), "word uppercase");
    const auto titlecase = Titlecase();
    WithinData(data, titlecase.data(), titlecase.size(), "word titlecase");
}

//-----------------------------------------------------------------------------
// FilteredDictionary
//-----------------------------------------------------------------------------
auto FilteredDictionary::operator[](IndexType index) const -> const WordEntry& {
    // index is a position into the filtered set [0, size); map it through the filtered
    // index sequence to a logical (lexicographic) word index, then to a word entry. Because
    // the index sequence is sorted, enumerating positions 0..size-1 yields words in
    // lexicographic order — matching the in-memory dictionary's generation order.
    auto logical_index = indices_.at(index);
    auto offset = (*index_table_)[logical_index];
    return word_data_->At(offset);
}

auto FilteredDictionary::ComputeMaxLength() const -> std::size_t {
    // Max byte length over the filtered words, matching the in-memory FilteredDictionary.
    // Visit logical indices directly (O(count)); the old per-position indices_.at(i) was an
    // O(ranges) scan, making this O(count * ranges) -- ~25s for a scattered exclude over a 1M-word
    // dictionary (e.g. {geo:-multi_token}).
    std::size_t max_length = 0;
    indices_.for_each([&](IndexType logical_index) {
        const auto offset = (*index_table_)[logical_index];
        max_length = std::max(max_length, word_data_->At(offset).Lowercase().size());
    });
    return max_length;
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
    filter_cache_ = std::make_shared<FilterCache>(kFilterCacheWays, kFilterCacheWaySize);

    Validate();
}

auto BinaryDictionary::Validate() const -> void {
    const auto word_count = index_table_->Count();
    // Every index entry must address a word entry whose fixed header and case strings are
    // in-bounds; walking all words validates the word-data region end to end.
    for (IndexType::UnderlyingType i = 0; i < word_count.GetUnderlying(); ++i) {
        const auto offset = (*index_table_)[IndexType{i}];
        word_data_->At(offset).Validate(data_);
    }
    language_table_->Validate(data_, word_count);
    tags_table_->Validate(data_, word_count);
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
    // Reuse the built filtered dictionary for identical selectors (same language/tags/size/case);
    // building it (filter + max-length) is the expensive part, so caching keeps repeated patterns
    // cheap under load. Keyed by the selector's identity hash, same as the in-memory path.
    const auto hash = selector.GetHash();
    if (auto cached = filter_cache_->Get(hash)) {
        return *cached;
    }
    auto index_sequence = language_table_->Filter(selector.language, selector.size_limit);
    auto indices = tags_table_->Filter(index_sequence, selector.include_tags, selector.exclude_tags);
    // TODO opt-in tags
    auto filtered = std::make_shared<FilteredDictionary>(indices, index_table_, word_data_, selector.GetCase());
    filter_cache_->Put(hash, filtered);
    return filtered;
}

//-----------------------------------------------------------------------------
// DictionarySet
//-----------------------------------------------------------------------------
void DictionarySet::Add(RawData data, std::shared_ptr<void> keepalive) {
    BinaryDictionary dictionary(data);
    auto kind = utils::text::ToLower(dictionary.Kind(), utils::text::kEnUsLocale);
    keepalives_.push_back(std::move(keepalive));
    dictionaries_.insert_or_assign(std::move(kind), std::move(dictionary));
}

auto DictionarySet::Filter(const Selector& selector) const -> FilteredDictionaryPtr {
    auto kind = utils::text::ToLower(selector.kind, utils::text::kEnUsLocale);
    auto it = dictionaries_.find(kind);
    if (it == dictionaries_.end()) {
        return {};
    }
    const auto& dictionary = it->second;
    const auto languages = dictionary.Languages();

    // Determine the effective language, mirroring the in-memory DictionarySet:
    //  - no language on the selector: prefer "en"; if the dictionary is language-agnostic
    //    (its words carry the empty language code, e.g. domain/shell), fall back to that;
    //  - an explicit language must be present in the dictionary, else the result is empty.
    // An unknown language yields an empty result rather than throwing.
    Selector effective = selector;
    if (!effective.language) {
        if (languages.find(LanguageCodeView{kDefaultLanguage}) != languages.end()) {
            effective.language = LanguageCodeView{kDefaultLanguage};
        } else if (languages.find(LanguageCodeView{kAgnosticLanguage}) != languages.end()) {
            effective.language = LanguageCodeView{kAgnosticLanguage};
        } else {
            return {};
        }
    } else if (languages.find(*effective.language) == languages.end()) {
        return {};
    }
    return dictionary.Filter(effective);
}

}  // namespace slugkit::generator::binary
