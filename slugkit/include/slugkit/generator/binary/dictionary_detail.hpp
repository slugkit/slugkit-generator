#pragma once

#include <slugkit/generator/filter/filtered_ids.hpp>
#include <slugkit/generator/placeholders.hpp>
#include <slugkit/generator/types.hpp>

#include <slugkit/utils/string_markup.hpp>

#include <userver/utils/strong_typedef.hpp>

#include <array>
#include <bit>
#include <cassert>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"
#pragma clang diagnostic ignored "-Wflexible-array-extensions"

namespace slugkit::generator::binary {

using RawData = std::span<const std::byte>;

namespace detail {

using StringMarkup =
    utils::basic_string_markup<std::string_view::value_type, std::uint16_t, std::string_view::traits_type>;

using OffsetType = userver::utils::
    StrongTypedef<class OffsetTypeTag, size_type, userver::utils::StrongTypedefOps::kCompareTransparent>;
namespace literals {

inline constexpr auto operator""_off(unsigned long long value) noexcept -> OffsetType {
    return OffsetType(static_cast<OffsetType::UnderlyingType>(value));
}

}  // namespace literals

inline auto Align(std::size_t size) -> std::size_t {
    auto pad_size = (sizeof(size_type) - size % sizeof(size_type)) % sizeof(size_type);
    return size + pad_size;
}

inline auto Align(const std::byte* base) -> const std::byte* {
    return reinterpret_cast<const std::byte*>(Align(reinterpret_cast<std::size_t>(base)));
}

inline auto Align(IndexType index) -> IndexType {
    auto pad_size = (sizeof(IndexType) - index.GetUnderlying() % sizeof(IndexType)) % sizeof(IndexType);
    return IndexType(index.GetUnderlying() + pad_size);
}

inline auto Align(OffsetType offset) -> OffsetType {
    auto pad_size = (sizeof(OffsetType) - offset.GetUnderlying() % sizeof(OffsetType)) % sizeof(OffsetType);
    return OffsetType(offset.GetUnderlying() + pad_size);
}

inline auto Align(SizeType size) -> SizeType {
    auto pad_size = (sizeof(SizeType) - size.GetUnderlying() % sizeof(SizeType)) % sizeof(SizeType);
    return SizeType(size.GetUnderlying() + pad_size);
}

/// @brief A no-alloc helper class.
/// @note The no-alloc helper class is used to avoid allocation of memory.
struct NoAlloc {
    NoAlloc() = delete;
    NoAlloc(const NoAlloc&) = delete;
    NoAlloc(NoAlloc&&) = delete;
    ~NoAlloc() = delete;
};

/// @brief A index range is a range of indexes.
/// @note The index range is immutable.
/// The index range is a range of word indexes that are stored in a contiguous array.
struct IndexRange final {
    [[nodiscard]] auto IsValid() const noexcept -> bool {
        return start_index < end_index;
    }

    /// @brief The size of the range index in bytes.
    [[nodiscard]] auto Size() const noexcept -> SizeType {
        return SizeType(sizeof(IndexType) * 2);
    }

    [[nodiscard]] auto Count() const noexcept -> IndexType {
        return end_index - start_index;
    }

    auto Filter() const noexcept -> filter::IndexRange {
        return {start_index, end_index};
    }

    IndexType start_index;
    IndexType end_index;
};

/// @brief A sparse index is a list of indexes.
/// @note The sparse index is immutable.
/// The sparse index is a list of word indexes that are not necessarily consecutive,
/// but are stored in a contiguous array. Indexes are sorted in ascending order.
class SparseIndex final : public NoAlloc {
public:
    using const_iterator = const IndexType*;

public:
    [[nodiscard]] auto Size() const noexcept -> SizeType {
        return SizeType(sizeof(IndexType) * count_.GetUnderlying() + 1);
    }

    [[nodiscard]] auto Count() const noexcept -> IndexType {
        return count_;
    }

    [[nodiscard]] auto operator[](IndexType index) const -> IndexType {
        if (index >= count_) {
            throw std::runtime_error(fmt::format("Index out of range [0, {}): {}", count_, index));
        }
        return indexes_[index.GetUnderlying()];
    }

    auto begin() const noexcept -> const_iterator {
        return indexes_;
    }

    auto end() const noexcept -> const_iterator {
        return indexes_ + count_.GetUnderlying();
    }

    auto Filter() const noexcept -> filter::IndexSet {
        return {indexes_, indexes_ + count_.GetUnderlying()};
    }

    static auto GetSparseIndex(RawData data) -> const SparseIndex*;
    static auto GetSparseIndex(const std::byte* base) -> const SparseIndex*;

private:
    IndexType count_;
    IndexType indexes_[];
};

/// @brief Length index, range of word with the same length.
/// @note The length index is immutable.
/// The length index is index range of words with the same length that are stored in a contiguous array.
/// Length index is a pair of length and index range.
struct LengthIndex final {
    SizeType length;
    IndexRange range;
};

/// @brief Table of length indexes.
/// @note The length index table is immutable.
/// The length index table is a table of length indexes that are stored in a contiguous array.
/// Length indexes are sorted by word length in ascending order.
class LengthIndexTable final : public NoAlloc {
public:
    using iterator = LengthIndex*;
    using const_iterator = const LengthIndex*;

public:
    static constexpr std::string_view kMagicNum = "LENGTH-INDEX====";

    [[nodiscard]] auto MagicNum() const noexcept -> std::string_view {
        return std::string_view{reinterpret_cast<const char*>(magic_num_.data()), magic_num_.size()};
    }

    [[nodiscard]] auto IsValid() const noexcept -> bool {
        return MagicNum() == kMagicNum;
    }

    [[nodiscard]] auto Size() const noexcept -> SizeType {
        return SizeType(kMagicNum.size() + sizeof(IndexType) + sizeof(LengthIndex) * count_.GetUnderlying());
    }

    [[nodiscard]] auto Count() const noexcept -> IndexType {
        return count_;
    }

    [[nodiscard]] auto Filter(SizeLimit size_limit) const -> filter::IndexRangeSequence;

    auto begin() const noexcept -> const_iterator {
        return length_indexes_;
    }

    auto end() const noexcept -> const_iterator {
        return length_indexes_ + count_.GetUnderlying();
    }

    auto front() const noexcept -> const LengthIndex& {
        return length_indexes_[0];
    }

    auto back() const noexcept -> const LengthIndex& {
        return length_indexes_[count_.GetUnderlying() - 1];
    }

private:
    std::array<char, kMagicNum.size()> magic_num_;
    IndexType count_;

    LengthIndex length_indexes_[];
};

/// @brief A header is a header of a binary dictionary file.
/// @note The header is immutable.
/// The header is reinterpret-cast from the beginning of the file.
class Header final : public NoAlloc {
public:
    static constexpr std::string_view kMagicNum = "SLUGDICT";
    static constexpr std::uint32_t kBinaryFormatVersion = 1;

    [[nodiscard]] auto IsValid() const noexcept -> bool {
        return MagicNum() == kMagicNum && binary_format_version_ == kBinaryFormatVersion;
    }

    [[nodiscard]] auto MagicNum() const noexcept -> std::string_view {
        return std::string_view{reinterpret_cast<const char*>(magic_num_.data()), magic_num_.size()};
    }

    /// @brief The size of the header in bytes.
    [[nodiscard]] auto Size() const noexcept -> SizeType {
        return GetSize();
    }

    [[nodiscard]] auto Kind() const noexcept -> std::string_view {
        return kind_.value(&data_start_);
    }

    [[nodiscard]] auto Version() const noexcept -> std::string_view {
        return version_.value(&data_start_);
    }

    [[nodiscard]] auto Description() const noexcept -> std::string_view {
        return description_.value(&data_start_);
    }

    static auto GetHeader(RawData data) -> const Header*;
    static auto GetHeader(const std::byte* base) -> const Header*;

private:
    [[nodiscard]] auto GetStringBase() const noexcept -> const std::byte* {
        return &data_start_;
    }

    [[nodiscard]] auto GetStringSize() const noexcept -> std::size_t {
        return kind_.size() + version_.size() + description_.size();
    }

    [[nodiscard]] auto GetSize() const noexcept -> SizeType {
        return SizeType(kFieldsSize + GetStringSize());
    }

    // string data is located after all header fields
    [[nodiscard]] auto GetStringData() const noexcept -> std::string_view {
        auto string_base = GetStringBase();
        auto string_size = GetStringSize();

        return {reinterpret_cast<const char*>(string_base), string_size};
    }

private:
    static constexpr std::size_t kFieldsSize = kMagicNum.size() + sizeof(std::uint32_t) + sizeof(StringMarkup) * 3;

    std::array<char, kMagicNum.size()> magic_num_;
    std::uint32_t binary_format_version_;
    StringMarkup kind_;
    StringMarkup version_;
    StringMarkup description_;
    std::byte data_start_;
};

/// @brief A index table is a table of offsets to word entries.
/// @note The index table is immutable.
class IndexTable final : public NoAlloc {
public:
    using iterator = OffsetType*;
    using const_iterator = const OffsetType*;

public:
    static constexpr std::string_view kMagicNum = "INDEX===";

    [[nodiscard]] auto MagicNum() const noexcept -> std::string_view {
        return std::string_view{reinterpret_cast<const char*>(magic_num_.data()), magic_num_.size()};
    }

    [[nodiscard]] auto IsValid() const noexcept -> bool {
        return MagicNum() == kMagicNum;
    }

    /// @brief The size of the index table in bytes.
    [[nodiscard]] auto Size() const noexcept -> SizeType {
        return SizeType(kMagicNum.size() + sizeof(SizeType) + count_.GetUnderlying() * sizeof(OffsetType));
    }

    [[nodiscard]] auto Count() const noexcept -> IndexType {
        return count_;
    }

    [[nodiscard]] auto operator[](IndexType index) const -> OffsetType {
        return index_table_[index.GetUnderlying()];
    }

    auto begin() const noexcept -> const_iterator {
        return index_table_;
    }

    auto end() const noexcept -> const_iterator {
        return index_table_ + count_.GetUnderlying();
    }

    static auto GetIndexTable(RawData data) -> const IndexTable*;
    static auto GetIndexTable(const std::byte* base) -> const IndexTable*;

private:
    std::array<char, kMagicNum.size()> magic_num_;
    IndexType count_;
    OffsetType index_table_[];
};

}  // namespace detail

class LanguageInfo final : public detail::NoAlloc {
public:
    static constexpr std::size_t kCodeSize = 4;

    [[nodiscard]] auto IsValid() const noexcept -> bool {
        return Code() != std::string_view{} && length_index_table_.IsValid();
    }

    [[nodiscard]] auto Code() const noexcept -> LanguageCodeView {
        // code is a null-terminated string
        return LanguageCodeView{reinterpret_cast<const char*>(code_.data())};
    }

    [[nodiscard]] auto Size() const noexcept -> SizeType {
        return SizeType(kCodeSize + sizeof(detail::IndexRange) + length_index_table_.Size().GetUnderlying());
    }

    [[nodiscard]] auto Count() const noexcept -> IndexType {
        return range_.Count();
    }

    [[nodiscard]] auto FullRange() const noexcept -> filter::IndexRangeSequence {
        return {range_.start_index, range_.end_index};
    }

    [[nodiscard]] auto Filter(SizeLimit size_limit) const noexcept -> filter::IndexRangeSequence {
        return length_index_table_.Filter(size_limit);
    }

    [[nodiscard]] auto Filter(std::optional<SizeLimit> size_limit) const noexcept -> filter::IndexRangeSequence {
        if (size_limit) {
            return length_index_table_.Filter(*size_limit);
        }
        return FullRange();
    }

    static auto GetLanguageInfo(RawData data) -> const LanguageInfo*;
    static auto GetLanguageInfo(const std::byte* base) -> const LanguageInfo*;

private:
    std::array<char, kCodeSize> code_;
    detail::IndexRange range_;
    detail::LengthIndexTable length_index_table_;
};

namespace detail {

/// @brief Table of language information.
class LanguageTable final : public NoAlloc {
public:
    class LanguageInfoIterator final {
    public:
        using difference_type = std::ptrdiff_t;

        LanguageInfoIterator(const OffsetType* offset, const std::byte* base)
            : offset_(offset)
            , base_(base) {
        }

        LanguageInfoIterator(const LanguageInfoIterator& other) noexcept = default;
        LanguageInfoIterator(LanguageInfoIterator&& other) noexcept = default;
        ~LanguageInfoIterator() = default;

        auto operator=(const LanguageInfoIterator& other) noexcept -> LanguageInfoIterator& = default;
        auto operator=(LanguageInfoIterator&& other) noexcept -> LanguageInfoIterator& = default;

        auto operator+=(difference_type n) noexcept -> LanguageInfoIterator& {
            offset_ += n;
            return *this;
        }
        auto operator-=(difference_type n) noexcept -> LanguageInfoIterator& {
            offset_ -= n;
            return *this;
        }

        auto operator*() const noexcept -> const LanguageInfo& {
            return *LanguageInfo::GetLanguageInfo(base_ + offset_->GetUnderlying());
        }

        auto operator->() const noexcept -> const LanguageInfo* {
            return LanguageInfo::GetLanguageInfo(base_ + offset_->GetUnderlying());
        }

        auto operator++() noexcept -> LanguageInfoIterator& {
            ++offset_;
            return *this;
        }

        auto operator++(int) noexcept -> LanguageInfoIterator {
            auto copy = *this;
            ++offset_;
            return copy;
        }

        auto operator--() noexcept -> LanguageInfoIterator& {
            --offset_;
            return *this;
        }

        auto operator--(int) noexcept -> LanguageInfoIterator {
            auto copy = *this;
            --offset_;
            return copy;
        }

        auto operator-(const LanguageInfoIterator& other) const noexcept -> difference_type {
            return offset_ - other.offset_;
        }

        auto operator+(difference_type n) const noexcept -> LanguageInfoIterator {
            return LanguageInfoIterator(offset_ + n, base_);
        }

        auto operator-(difference_type n) const noexcept -> LanguageInfoIterator {
            return LanguageInfoIterator(offset_ - n, base_);
        }

        auto operator==(const LanguageInfoIterator& other) const noexcept -> bool {
            return offset_ == other.offset_;
        }

        auto operator!=(const LanguageInfoIterator& other) const noexcept -> bool {
            return offset_ != other.offset_;
        }

    private:
        const OffsetType* offset_;
        const std::byte* base_;
    };

    using const_iterator = LanguageInfoIterator;

public:
    static constexpr std::string_view kMagicNum = "LANG-TABLE======";

    [[nodiscard]] auto MagicNum() const noexcept -> std::string_view {
        return std::string_view{reinterpret_cast<const char*>(magic_num_.data()), magic_num_.size()};
    }

    [[nodiscard]] auto IsValid() const noexcept -> bool {
        return MagicNum() == kMagicNum;
    }

    [[nodiscard]] auto Size() const noexcept -> SizeType {
        return size_;
    }

    [[nodiscard]] auto Count() const noexcept -> IndexType {
        return count_;
    }

    [[nodiscard]] auto Languages() const noexcept -> LanguageCodeSet;

    [[nodiscard]] auto Filter(std::optional<LanguageCodeView> language, std::optional<SizeLimit> size_limit) const
        -> filter::IndexRangeSequence;

    [[nodiscard]] auto operator[](IndexType index) const -> const LanguageInfo& {
        return *LanguageInfo::GetLanguageInfo(GetLanguageBase(index));
    }

    auto begin() const noexcept -> const_iterator {
        return LanguageInfoIterator(GetOffsetTableBase(), GetLanguageInfoTableBase());
    }

    auto end() const noexcept -> const_iterator {
        return LanguageInfoIterator(GetOffsetTableBase() + count_.GetUnderlying(), GetLanguageInfoTableBase());
    }

    auto operator[](LanguageCodeView language) const -> const LanguageInfo&;

    static auto GetLanguageTable(RawData data) -> const LanguageTable*;
    static auto GetLanguageTable(const std::byte* base) -> const LanguageTable*;

private:
    [[nodiscard]] auto GetOffsetTableBase() const noexcept -> const OffsetType* {
        return reinterpret_cast<const OffsetType*>(&data_start_);
    }

    [[nodiscard]] auto GetLanguageInfoTableBase() const noexcept -> const std::byte* {
        return reinterpret_cast<const std::byte*>(GetOffsetTableBase() + count_.GetUnderlying());
    }

    [[nodiscard]] auto GetLanguageOffset(IndexType index) const -> OffsetType {
        if (index >= count_) {
            throw std::runtime_error(fmt::format("Invalid language index: {}", index.GetUnderlying()));
        }
        return GetOffsetTableBase()[index.GetUnderlying()];
    }

    [[nodiscard]] auto GetLanguageBase(IndexType index) const -> const std::byte* {
        return GetLanguageInfoTableBase() + GetLanguageOffset(index).GetUnderlying();
    }

private:
    std::array<char, kMagicNum.size()> magic_num_;
    SizeType size_;
    IndexType count_;
    // first here is the language info offset table, next is the language info table
    std::byte data_start_;
};

}  // namespace detail

/// @brief A tag entry is a tag description and sparse index of words that have the tag.
/// @note The tag entry is immutable.
class TagEntry final : public detail::NoAlloc {
public:
    using const_iterator = detail::SparseIndex::const_iterator;

public:
    static constexpr std::string_view kMagicNum = "TAG=====";

    [[nodiscard]] auto MagicNum() const noexcept -> std::string_view {
        return std::string_view{magic_num_.data(), magic_num_.size()};
    }

    [[nodiscard]] auto IsValid() const noexcept -> bool {
        return MagicNum() == kMagicNum;
    }

    [[nodiscard]] auto Name() const noexcept -> TagView {
        return TagView{name_.value(&data_start_)};
    }

    [[nodiscard]] auto Description() const noexcept -> std::string_view {
        return description_.value(&data_start_);
    }

    [[nodiscard]] auto OptIn() const noexcept -> bool {
        return opt_in_ == std::byte{1};
    }

    [[nodiscard]] auto Index() const noexcept -> const detail::SparseIndex& {
        return *detail::SparseIndex::GetSparseIndex(GetIndexBase());
    }

    [[nodiscard]] auto Count() const noexcept -> IndexType {
        return Index().Count();
    }

    [[nodiscard]] auto Filter() const noexcept -> filter::IndexSetView {
        return {begin(), end()};
    }

    auto begin() const noexcept -> const_iterator {
        return Index().begin();
    }

    auto end() const noexcept -> const_iterator {
        return Index().end();
    }

    [[nodiscard]] auto operator[](IndexType index) const -> IndexType {
        return Index()[index];
    }

    static auto GetTagEntry(RawData data) -> const TagEntry*;
    static auto GetTagEntry(const std::byte* base) -> const TagEntry*;

private:
    [[nodiscard]] auto GetStringBase() const noexcept -> const std::byte* {
        return &data_start_;
    }

    [[nodiscard]] auto GetStringSize() const noexcept -> std::size_t {
        return name_.size() + description_.size();
    }

    [[nodiscard]] auto GetStringData() const noexcept -> std::string_view {
        auto string_base = GetStringBase();
        auto string_size = GetStringSize();

        return {reinterpret_cast<const char*>(string_base), string_size};
    }

    [[nodiscard]] auto GetIndexBase() const noexcept -> const std::byte* {
        auto string_size = GetStringSize();
        auto base = &data_start_ + string_size;
        auto aligned_base = detail::Align(base);
        return aligned_base;
    }

private:
    std::array<char, kMagicNum.size()> magic_num_;
    detail::StringMarkup name_;
    detail::StringMarkup description_;
    std::byte opt_in_;
    std::byte data_start_;
};

namespace detail {

class TagsTable final : public NoAlloc {
public:
    class TagEntryIterator final {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = TagEntry;
        using pointer = const TagEntry*;
        using reference = const TagEntry&;

        TagEntryIterator(const OffsetType* offset, const std::byte* base)
            : offset_(offset)
            , base_(base) {
        }

        TagEntryIterator(const TagEntryIterator& other) noexcept = default;
        TagEntryIterator(TagEntryIterator&& other) noexcept = default;
        ~TagEntryIterator() = default;

        auto operator=(const TagEntryIterator& other) noexcept -> TagEntryIterator& = default;
        auto operator=(TagEntryIterator&& other) noexcept -> TagEntryIterator& = default;

        auto operator+=(difference_type n) noexcept -> TagEntryIterator& {
            offset_ += n;
            return *this;
        }
        auto operator-=(difference_type n) noexcept -> TagEntryIterator& {
            offset_ -= n;
            return *this;
        }

        auto operator==(const TagEntryIterator& other) const noexcept -> bool {
            assert(base_ == other.base_);
            return offset_ == other.offset_;
        }

        auto operator!=(const TagEntryIterator& other) const noexcept -> bool {
            assert(base_ == other.base_);
            return offset_ != other.offset_;
        }

        auto operator*() const noexcept -> reference {
            return *TagEntry::GetTagEntry(base_ + offset_->GetUnderlying());
        }

        auto operator->() const noexcept -> pointer {
            return TagEntry::GetTagEntry(base_ + offset_->GetUnderlying());
        }

        auto operator++() noexcept -> TagEntryIterator& {
            ++offset_;
            return *this;
        }

        auto operator++(int) noexcept -> TagEntryIterator {
            auto copy = *this;
            ++offset_;
            return copy;
        }

        auto operator--() noexcept -> TagEntryIterator& {
            --offset_;
            return *this;
        }

        auto operator--(int) noexcept -> TagEntryIterator {
            auto copy = *this;
            --offset_;
            return copy;
        }

        auto operator-(const TagEntryIterator& other) const noexcept -> difference_type {
            assert(base_ == other.base_);
            return offset_ - other.offset_;
        }

        auto operator+(difference_type n) const noexcept -> TagEntryIterator {
            return TagEntryIterator(offset_ + n, base_);
        }

        auto operator-(difference_type n) const noexcept -> TagEntryIterator {
            return TagEntryIterator(offset_ - n, base_);
        }

        auto operator[](difference_type n) const noexcept -> reference {
            return *(*this + n);
        }

        auto operator<(const TagEntryIterator& other) const noexcept -> bool {
            assert(base_ == other.base_);
            return offset_ < other.offset_;
        }

        auto operator>(const TagEntryIterator& other) const noexcept -> bool {
            assert(base_ == other.base_);
            return offset_ > other.offset_;
        }

        auto operator<=(const TagEntryIterator& other) const noexcept -> bool {
            assert(base_ == other.base_);
            return offset_ <= other.offset_;
        }

        auto operator>=(const TagEntryIterator& other) const noexcept -> bool {
            assert(base_ == other.base_);
            return offset_ >= other.offset_;
        }

        friend auto operator+(difference_type n, const TagEntryIterator& it) noexcept -> TagEntryIterator {
            return it + n;
        }

    private:
        const OffsetType* offset_;
        const std::byte* base_;
    };

    using const_iterator = TagEntryIterator;

public:
    static constexpr std::string_view kMagicNum = "TAGS-TABLE======";

    [[nodiscard]] auto MagicNum() const noexcept -> std::string_view {
        return std::string_view{magic_num_.data(), magic_num_.size()};
    }

    [[nodiscard]] auto IsValid() const noexcept -> bool {
        return MagicNum() == kMagicNum;
    }

    [[nodiscard]] auto Size() const noexcept -> SizeType {
        return size_;
    }

    [[nodiscard]] auto Count() const noexcept -> IndexType {
        return count_;
    }

    [[nodiscard]] auto Tags() const noexcept -> TagSet;

    /// @brief Filter the tags table based on the index range sequence, include tags, and exclude tags.
    /// @param index_range_sequence The index range sequence to filter the tags table. Calcluated based on language
    /// and length limits.
    /// @param include_tags The include tags to filter the tags table.
    /// @param exclude_tags The exclude tags to filter the tags table.
    /// @return The index set of the filtered tags table.
    /// Filter is:
    /// Intersection of include tags and difference of exclude tags from include tags
    /// Empty tag list - all tags
    [[nodiscard]] auto Filter(
        const filter::IndexRangeSequence& index_range_sequence,
        const TagSet& include_tags,
        const TagSet& exclude_tags
    ) const -> filter::IndexSequence;

    [[nodiscard]] auto operator[](IndexType index) const -> const TagEntry& {
        return *TagEntry::GetTagEntry(GetTagDataBase() + GetTagOffset(index).GetUnderlying());
    }

    auto operator[](TagView tag) const -> const TagEntry&;
    auto operator[](Tag tag) const -> const TagEntry&;

    auto begin() const noexcept -> const_iterator {
        return TagEntryIterator(GetOffsetTableBase(), GetTagDataBase());
    }

    auto end() const noexcept -> const_iterator {
        return TagEntryIterator(GetOffsetTableBase() + count_.GetUnderlying(), GetTagDataBase());
    }

    static auto GetTagsTable(RawData data) -> const TagsTable*;
    static auto GetTagsTable(const std::byte* base) -> const TagsTable*;

private:
    [[nodiscard]] auto GetOffsetTableBase() const noexcept -> const OffsetType* {
        return reinterpret_cast<const OffsetType*>(&data_start_);
    }

    [[nodiscard]] auto GetTagDataBase() const noexcept -> const std::byte* {
        return reinterpret_cast<const std::byte*>(GetOffsetTableBase() + count_.GetUnderlying());
    }

    [[nodiscard]] auto GetTagOffset(IndexType index) const -> OffsetType {
        if (index >= count_) {
            throw std::runtime_error(fmt::format("Invalid tag index: {}", index.GetUnderlying()));
        }
        return GetOffsetTableBase()[index.GetUnderlying()];
    }

    [[nodiscard]] auto FindTagEntry(TagView tag) const -> const TagEntry*;
    [[nodiscard]] auto GetTagFilter(TagView tag) const -> filter::IndexSetView;

private:
    std::array<char, kMagicNum.size()> magic_num_;
    SizeType size_;
    IndexType count_;
    // first here is the tag data offset table, next is the tag data table
    std::byte data_start_;
};

}  // namespace detail

/// @brief A word entry is a word entry in a binary dictionary file.
/// @note The word entry is immutable.
class WordEntry final : public detail::NoAlloc {
public:
    static auto GetWordEntry(RawData data) -> const WordEntry* {
        if (data.size() < sizeof(WordEntry)) {
            throw std::runtime_error(fmt::format(
                "Invalid data: size {} is less than word entry expected size {}", data.size(), sizeof(WordEntry)
            ));
        }
        return GetWordEntry(data.data());
    }

    static auto GetWordEntry(const std::byte* base) -> const WordEntry* {
        return reinterpret_cast<const WordEntry*>(base);
    }

    [[nodiscard]] auto Lowercase() const noexcept -> std::string_view {
        return lowercase_.value(&data_start_);
    }

    [[nodiscard]] auto Uppercase() const noexcept -> std::string_view {
        return uppercase_.value(&data_start_);
    }

    [[nodiscard]] auto Titlecase() const noexcept -> std::string_view {
        return titlecase_.value(&data_start_);
    }

    [[nodiscard]] auto Size() const noexcept -> SizeType {
        return GetSize();
    }

private:
    constexpr static std::size_t kFieldsSize = sizeof(detail::StringMarkup) * 3;
    [[nodiscard]] auto GetBasePointer() const noexcept -> const std::byte* {
        return reinterpret_cast<const std::byte*>(this);
    }

    [[nodiscard]] auto GetStringBase() const noexcept -> const std::byte* {
        return &data_start_;
    }

    [[nodiscard]] auto GetStringSize() const noexcept -> std::size_t {
        return lowercase_.size() + uppercase_.size() + titlecase_.size();
    }

    [[nodiscard]] auto GetSize() const noexcept -> SizeType {
        return SizeType(kFieldsSize + GetStringSize());
    }

    [[nodiscard]] auto GetStringData() const noexcept -> std::string_view {
        auto string_base = GetStringBase();
        auto string_size = GetStringSize();

        return {reinterpret_cast<const char*>(string_base), string_size};
    }

    detail::StringMarkup lowercase_;
    detail::StringMarkup uppercase_;
    detail::StringMarkup titlecase_;
    std::byte data_start_;
};

namespace detail {

/// @brief A word data is a list of word entries.
/// @note The word data is immutable.
class WordData final : public NoAlloc {
public:
    static constexpr std::string_view kMagicNum = "WORDS===";

    [[nodiscard]] auto IsValid() const noexcept -> bool {
        return MagicNum() == kMagicNum;
    }

    [[nodiscard]] auto MagicNum() const noexcept -> std::string_view {
        return std::string_view{magic_num_.data(), magic_num_.size()};
    }

    /// @brief The size of the word data section in bytes.
    /// Includes the magic number, size, count, and data section.
    [[nodiscard]] auto Size() const noexcept -> SizeType {
        return size_;
    }

    [[nodiscard]] auto Count() const noexcept -> IndexType {
        return count_;
    }

    [[nodiscard]] auto At(OffsetType offset) const -> const WordEntry& {
        if (offset.GetUnderlying() >= size_.GetUnderlying()) {
            throw std::runtime_error(fmt::format("Invalid offset: 0x{:x}", offset.GetUnderlying()));
        }
        return *WordEntry::GetWordEntry(&data_start_ + offset.GetUnderlying());
    }

    static auto GetWordData(RawData data) -> const WordData*;
    static auto GetWordData(const std::byte* base) -> const WordData*;

private:
    std::array<char, kMagicNum.size()> magic_num_;
    SizeType size_;
    IndexType count_;
    std::byte data_start_;
};

}  // namespace detail

// ---------------------------------------------------------------------------------------
// Wire-format layout locks.
//
// The structures above are reinterpret_cast directly over memory-mapped file bytes, so
// their in-memory layout *is* the on-disk format. These compile-time assertions fail the
// build if a field type, order, size, or padding changes in a way that would silently
// corrupt parsing — a class of bug the runtime magic/bounds checks cannot catch — and pin
// the format to little-endian hosts (the parser does no byte-swapping).
// ---------------------------------------------------------------------------------------

static_assert(
    std::endian::native == std::endian::little,
    "binary dictionary format is little-endian; big-endian hosts are unsupported"
);

// Fixed-size scalar building blocks (the format spec sizes).
static_assert(sizeof(IndexType) == 4, "IndexType must be 4 bytes");
static_assert(sizeof(SizeType) == 4, "SizeType must be 4 bytes");
static_assert(sizeof(detail::OffsetType) == 4, "OffsetType must be 4 bytes");
static_assert(sizeof(detail::StringMarkup) == 4, "StringMarkup is two uint16_t (offset, size)");
static_assert(sizeof(detail::IndexRange) == 8, "IndexRange is two IndexType");
static_assert(sizeof(detail::LengthIndex) == 12, "LengthIndex is SizeType + IndexRange");

// Every wire struct must be standard-layout: reinterpret_cast from raw bytes is only
// well-defined for standard-layout types.
static_assert(std::is_standard_layout_v<detail::IndexRange>);
static_assert(std::is_standard_layout_v<detail::LengthIndex>);
static_assert(std::is_standard_layout_v<detail::SparseIndex>);
static_assert(std::is_standard_layout_v<detail::LengthIndexTable>);
static_assert(std::is_standard_layout_v<detail::Header>);
static_assert(std::is_standard_layout_v<detail::IndexTable>);
static_assert(std::is_standard_layout_v<detail::LanguageTable>);
static_assert(std::is_standard_layout_v<detail::TagsTable>);
static_assert(std::is_standard_layout_v<detail::WordData>);
static_assert(std::is_standard_layout_v<LanguageInfo>);
static_assert(std::is_standard_layout_v<TagEntry>);
static_assert(std::is_standard_layout_v<WordEntry>);

// Section magic numbers must match their documented on-disk widths.
static_assert(detail::Header::kMagicNum.size() == 8, "SLUGDICT");
static_assert(detail::IndexTable::kMagicNum.size() == 8, "INDEX===");
static_assert(detail::WordData::kMagicNum.size() == 8, "WORDS===");
static_assert(TagEntry::kMagicNum.size() == 8, "TAG=====");
static_assert(detail::LanguageTable::kMagicNum.size() == 16, "LANG-TABLE======");
static_assert(detail::TagsTable::kMagicNum.size() == 16, "TAGS-TABLE======");
static_assert(detail::LengthIndexTable::kMagicNum.size() == 16, "LENGTH-INDEX====");

}  // namespace slugkit::generator::binary

#pragma clang diagnostic pop
