#pragma once

#include <slugkit/generator/binary/dictionary_detail.hpp>
#include <slugkit/generator/dictionary_types.hpp>
#include <slugkit/generator/pattern.hpp>

#include <fmt/format.h>

/*
See docs/binary_dictionary_format.md for the file layout.
*/

namespace slugkit::generator::binary {

class FilteredDictionary {
public:
    FilteredDictionary(
        filter::IndexSequence indices,
        const detail::IndexTable* index_table,
        const detail::WordData* word_data
    )
        : indices_(std::move(indices))
        , index_table_(index_table)
        , word_data_(word_data) {
        assert(index_table_ != nullptr);
        assert(word_data_ != nullptr);
    }

    ~FilteredDictionary() = default;

    auto size() const -> std::size_t {
        return indices_.count().GetUnderlying();
    }

    auto empty() const -> bool {
        return indices_.empty();
    }

    auto operator[](IndexType index) const -> const WordEntry&;

private:
    filter::IndexSequence indices_;
    const detail::IndexTable* index_table_;
    const detail::WordData* word_data_;
};

using FilteredDictionaryPtr = std::shared_ptr<FilteredDictionary>;

/// @brief A binary dictionary is an interface to a binary memory-mapped dictionary file
///        or a memory region that contains a dictionary.
class BinaryDictionary {
public:
    using RawData = std::span<const std::byte>;

public:
    BinaryDictionary(RawData data);

    [[nodiscard]] auto Kind() const noexcept -> const std::string_view {
        return header_->Kind();
    }

    [[nodiscard]] auto Version() const noexcept -> const std::string_view {
        return header_->Version();
    }

    [[nodiscard]] auto Count() const noexcept -> IndexType {
        return index_table_->Count();
    }

    [[nodiscard]] auto Languages() const noexcept -> LanguageCodeSet {
        return language_table_->Languages();
    }

    [[nodiscard]] auto Tags() const noexcept -> TagSet {
        return tags_table_->Tags();
    }

    auto Filter(const Selector& selector) const -> FilteredDictionaryPtr;

    auto operator[](LanguageCodeView language) const -> const LanguageInfo&;
    auto operator[](Tag tag) const -> const TagEntry&;
    auto operator[](TagView tag) const -> const TagEntry&;

    auto operator[](IndexType index) const -> const WordEntry&;

private:
    RawData data_;
    const detail::Header* header_;
    const detail::IndexTable* index_table_;
    const detail::LanguageTable* language_table_;
    const detail::TagsTable* tags_table_;
    const detail::WordData* word_data_;
};

}  // namespace slugkit::generator::binary
