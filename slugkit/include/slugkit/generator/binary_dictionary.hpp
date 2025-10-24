#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"
#pragma clang diagnostic ignored "-Wflexible-array-extensions"

#include <slugkit/utils/string_markup.hpp>

#include <userver/utils/strong_typedef.hpp>

#include <array>
#include <span>
#include <stdexcept>
#include <string_view>

#include <fmt/format.h>

/*
See docs/binary_dictionary_format.md for the file layout.
*/

namespace slugkit::generator::binary {

using RawData = std::span<const std::byte>;
using StringMarkup =
    utils::basic_string_markup<std::string_view::value_type, std::uint16_t, std::string_view::traits_type>;

using size_type = std::uint32_t;

using OffsetType = userver::utils::
    StrongTypedef<class OffsetTypeTag, size_type, userver::utils::StrongTypedefOps::kCompareTransparent>;
using IndexType =
    userver::utils::StrongTypedef<class IndexTypeTag, size_type, userver::utils::StrongTypedefOps::kCompareTransparent>;
using SizeType =
    userver::utils::StrongTypedef<class SizeTypeTag, size_type, userver::utils::StrongTypedefOps::kCompareTransparent>;

inline constexpr auto operator+(IndexType lhs, IndexType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() + rhs.GetUnderlying());
}
inline constexpr auto operator+(IndexType lhs, IndexType::UnderlyingType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() + rhs);
}
inline constexpr auto operator-(IndexType lhs, IndexType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() - rhs.GetUnderlying());
}
inline constexpr auto operator-(IndexType lhs, IndexType::UnderlyingType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() - rhs);
}
inline constexpr auto operator*(IndexType lhs, IndexType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() * rhs.GetUnderlying());
}
inline constexpr auto operator/(IndexType lhs, IndexType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() / rhs.GetUnderlying());
}
inline constexpr auto operator/(IndexType lhs, IndexType::UnderlyingType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() / rhs);
}
inline constexpr auto operator%(IndexType lhs, IndexType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() % rhs.GetUnderlying());
}

class IndexTable;
class WordEntry;

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

struct OffsetAndSize final {
    OffsetType offset;
    SizeType size;

    [[nodiscard]] auto empty() const noexcept -> bool {
        return size.GetUnderlying() == 0;
    }

    [[nodiscard]] auto GetData(RawData data) const -> RawData {
        return data.subspan(offset.GetUnderlying(), size.GetUnderlying());
    }

    [[nodiscard]] auto GetData(const std::byte* base) const -> RawData {
        return {base + offset.GetUnderlying(), size.GetUnderlying()};
    }
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

    auto begin() const noexcept -> const_iterator {
        return length_indexes_;
    }

    auto end() const noexcept -> const_iterator {
        return length_indexes_ + count_.GetUnderlying();
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
        return SizeType(kMagicNum.size() + sizeof(SizeType) + size_.GetUnderlying() * sizeof(OffsetType));
    }

    [[nodiscard]] auto Count() const noexcept -> IndexType {
        return size_;
    }

    [[nodiscard]] auto operator[](IndexType index) const -> OffsetType {
        return index_table_[index.GetUnderlying()];
    }

    auto begin() const noexcept -> const_iterator {
        return index_table_;
    }

    auto end() const noexcept -> const_iterator {
        return index_table_ + size_.GetUnderlying();
    }

    static auto GetIndexTable(RawData data) -> const IndexTable*;
    static auto GetIndexTable(const std::byte* base) -> const IndexTable*;

private:
    std::array<char, kMagicNum.size()> magic_num_;
    IndexType size_;
    OffsetType index_table_[];
};

class LanguageInfo final : public NoAlloc {
public:
    static constexpr std::size_t kCodeSize = 4;

    [[nodiscard]] auto IsValid() const noexcept -> bool {
        return Code() != std::string_view{} && length_index_table_.IsValid();
    }

    [[nodiscard]] auto Code() const noexcept -> std::string_view {
        // code is a null-terminated string
        return std::string_view{reinterpret_cast<const char*>(code_.data())};
    }

    [[nodiscard]] auto Size() const noexcept -> SizeType {
        return SizeType(kCodeSize + sizeof(IndexRange) + length_index_table_.Size().GetUnderlying());
    }

    [[nodiscard]] auto Count() const noexcept -> IndexType {
        return range_.Count();
    }

    static auto GetLanguageInfo(RawData data) -> const LanguageInfo*;
    static auto GetLanguageInfo(const std::byte* base) -> const LanguageInfo*;

private:
    std::array<char, kCodeSize> code_;
    IndexRange range_;
    LengthIndexTable length_index_table_;
};

/// @brief Table of language information.
class LanguageTable final : public NoAlloc {
public:
    using iterator = LanguageInfo*;
    using const_iterator = const LanguageInfo*;

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

    [[nodiscard]] auto operator[](IndexType index) const -> const LanguageInfo& {
        return *LanguageInfo::GetLanguageInfo(GetLanguageBase(index));
    }

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

/// @brief A tag entry is a tag description and sparse index of words that have the tag.
/// @note The tag entry is immutable.
class TagEntry final : public NoAlloc {
public:
    using const_iterator = SparseIndex::const_iterator;

public:
    static constexpr std::string_view kMagicNum = "TAG=====";

    [[nodiscard]] auto MagicNum() const noexcept -> std::string_view {
        return std::string_view{magic_num_.data(), magic_num_.size()};
    }

    [[nodiscard]] auto IsValid() const noexcept -> bool {
        return MagicNum() == kMagicNum;
    }

    [[nodiscard]] auto Name() const noexcept -> std::string_view {
        return name_.value(&data_start_);
    }

    [[nodiscard]] auto Description() const noexcept -> std::string_view {
        return description_.value(&data_start_);
    }

    [[nodiscard]] auto OptIn() const noexcept -> bool {
        return opt_in_ == std::byte{1};
    }

    [[nodiscard]] auto Index() const noexcept -> const SparseIndex& {
        return *SparseIndex::GetSparseIndex(GetIndexBase());
    }

    [[nodiscard]] auto Count() const noexcept -> IndexType {
        return Index().Count();
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
        auto aligned_base = Align(base);
        return aligned_base;
    }

private:
    std::array<char, kMagicNum.size()> magic_num_;
    StringMarkup name_;
    StringMarkup description_;
    std::byte opt_in_;
    std::byte data_start_;
};

class TagsTable final : public NoAlloc {
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

    [[nodiscard]] auto operator[](IndexType index) const -> const TagEntry& {
        return *TagEntry::GetTagEntry(GetTagDataBase() + GetTagOffset(index).GetUnderlying());
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

private:
    std::array<char, kMagicNum.size()> magic_num_;
    SizeType size_;
    IndexType count_;
    // first here is the tag data offset table, next is the tag data table
    std::byte data_start_;
};

/// @brief A word entry is a word entry in a binary dictionary file.
/// @note The word entry is immutable.
class WordEntry final : public NoAlloc {
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
    constexpr static std::size_t kFieldsSize = sizeof(StringMarkup) * 3;
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

    StringMarkup lowercase_;
    StringMarkup uppercase_;
    StringMarkup titlecase_;
    std::byte data_start_;
};

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

    [[nodiscard]] auto At(OffsetType offset) const -> const WordEntry& {
        return *WordEntry::GetWordEntry(&data_start_ + offset.GetUnderlying());
    }

    static auto GetWordData(RawData data) -> const WordData*;
    static auto GetWordData(const std::byte* base) -> const WordData*;

private:
    std::array<char, kMagicNum.size()> magic_num_;
    std::byte data_start_;
};

/// @brief A binary dictionary is an interface to a binary memory-mapped dictionary file
///        or a memory region that contains a dictionary.
class BinaryDictionary {
public:
    using RawData = std::span<const std::byte>;

public:
    BinaryDictionary(RawData data)
        : data_(data) {
        header_ = Header::GetHeader(data_);
        auto consumed_size = Align(header_->Size()).GetUnderlying();

        index_table_ = IndexTable::GetIndexTable(data_.subspan(consumed_size));
        consumed_size += Align(index_table_->Size()).GetUnderlying();

        language_table_ = LanguageTable::GetLanguageTable(data_.subspan(consumed_size));
        consumed_size += Align(language_table_->Size()).GetUnderlying();

        tags_table_ = TagsTable::GetTagsTable(data_.subspan(consumed_size));
        consumed_size += Align(tags_table_->Size()).GetUnderlying();

        word_data_ = WordData::GetWordData(data_.subspan(consumed_size));
    }

private:
    RawData data_;
    const Header* header_;
    const IndexTable* index_table_;
    const LanguageTable* language_table_;
    const TagsTable* tags_table_;
    const WordData* word_data_;
};

}  // namespace slugkit::generator::binary

#pragma clang diagnostic pop
