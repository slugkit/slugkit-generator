#pragma once

#include <slugkit/generator/binary/dictionary_detail.hpp>
#include <slugkit/generator/dictionary_types.hpp>
#include <slugkit/generator/mixed_case_index.hpp>
#include <slugkit/generator/pattern.hpp>

#include <slugkit/compat/lru_cache.hpp>

#include <fmt/format.h>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/*
See docs/binary_dictionary_format.md for the file layout.
*/

namespace slugkit::generator::binary {

class FilteredDictionary {
public:
    FilteredDictionary(
        filter::IndexSequence indices,
        const detail::IndexTable* index_table,
        const detail::WordData* word_data,
        CaseType case_type = CaseType::kNone
    )
        : indices_(std::move(indices))
        , index_table_(index_table)
        , word_data_(word_data)
        , case_type_(case_type)
        , max_length_(ComputeMaxLength()) {
        assert(index_table_ != nullptr);
        assert(word_data_ != nullptr);
        if (case_type_ == CaseType::kMixed) {
            BuildMixedIndex();
        }
    }

    ~FilteredDictionary() = default;

    auto size() const -> std::size_t {
        return indices_.count().GetUnderlying();
    }

    auto empty() const -> bool {
        return indices_.empty();
    }

    /// @brief The selector's case; the substitution generator picks the matching precomputed
    /// word variant (kMixed uses the lowercase variant plus a per-word case mask).
    [[nodiscard]] auto GetCase() const noexcept -> CaseType {
        return case_type_;
    }

    /// @brief The maximum byte length among the filtered words (matches the in-memory
    /// FilteredDictionary: max over the filtered set, used for the mixed-case mask and the
    /// pattern's max-length estimate).
    [[nodiscard]] auto GetMaxLength() const noexcept -> std::size_t {
        return max_length_;
    }

    /// @brief Mixed-case capacity: the number of distinct cased forms across all filtered words.
    /// Only meaningful when GetCase() == kMixed; otherwise the layout is empty and this is 0.
    [[nodiscard]] auto MixedCapacity() const noexcept -> std::uint64_t {
        return mixed_index_.Capacity();
    }

    /// @brief Decompose a permuted sequence value into (word index, compact case index) for the
    /// mixed-case path. @see MixedCaseIndex.
    [[nodiscard]] auto DecomposeMixed(std::uint64_t value) const -> std::pair<std::size_t, std::uint64_t> {
        return mixed_index_.Decompose(value);
    }

    auto operator[](IndexType index) const -> const WordEntry&;

private:
    auto ComputeMaxLength() const -> std::size_t;
    void BuildMixedIndex();

    filter::IndexSequence indices_;
    const detail::IndexTable* index_table_;
    const detail::WordData* word_data_;
    CaseType case_type_;
    std::size_t max_length_;
    // Built only for kMixed selectors; maps sequence values to (word, case mask) without collisions.
    MixedCaseIndex mixed_index_;
};

using FilteredDictionaryPtr = std::shared_ptr<FilteredDictionary>;
using FilteredDictionaryConstPtr = std::shared_ptr<const FilteredDictionary>;

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

    /// @brief Filter the dictionary by a selector.
    /// @param enabled_opt_ins Tags whose opt-in gate is lifted for this request (the "honest
    /// opt-in" usage flag). A word carrying an opt-in tag is hidden unless that tag is either
    /// requested explicitly via the selector's include tags or listed here.
    auto Filter(const Selector& selector, const TagSet& enabled_opt_ins = {}) const -> FilteredDictionaryPtr;

    auto operator[](LanguageCodeView language) const -> const LanguageInfo&;
    auto operator[](Tag tag) const -> const TagEntry&;
    auto operator[](TagView tag) const -> const TagEntry&;

    auto operator[](IndexType index) const -> const WordEntry&;

private:
    /// @brief Recursively validate the whole dictionary against the backing data: every word
    /// entry, language info, and tag entry, and that all referenced word indexes are in
    /// range. Runs once at construction; throws DictionaryDataError on any violation.
    auto Validate() const -> void;

private:
    // Per-selector cache of built filtered dictionaries, keyed by Selector::GetHash(). The binary
    // Filter path otherwise rebuilds the whole FilteredDictionary every request; this mirrors the
    // in-memory path's FilteredDictionaryCache (userver thread-safe sharded LRU). Held by
    // shared_ptr so BinaryDictionary stays movable (it's stored by value in DictionarySet).
    using FilterCache = slugkit::compat::NWayLRU<std::int64_t, FilteredDictionaryPtr>;
    static constexpr std::size_t kFilterCacheWays = 16UL;
    static constexpr std::size_t kFilterCacheWaySize = 1024UL;

    RawData data_;
    const detail::Header* header_;
    const detail::IndexTable* index_table_;
    const detail::LanguageTable* language_table_;
    const detail::TagsTable* tags_table_;
    const detail::WordData* word_data_;
    std::shared_ptr<FilterCache> filter_cache_;
    // Names of tags flagged opt-in (TagEntry::OptIn()), viewing the mmap'd tag strings.
    // Precomputed once so Filter subtracts only these (few) tags rather than rescanning the
    // whole tags table (thousands of entries for the emoji dictionary) on every request.
    std::vector<TagView> opt_in_tags_;
};

/// @brief A set of binary dictionaries keyed by kind, the binary counterpart of
/// generator::DictionarySet. Each BinaryDictionary already holds all languages for its kind,
/// so the set keys by kind only and defers language selection to the dictionary.
/// @note The set retains a keepalive for each dictionary's backing bytes (e.g. a memory
/// mapping), so the mapped data outlives the dictionaries that view it.
class DictionarySet {
public:
    using RawData = BinaryDictionary::RawData;

    DictionarySet() = default;

    /// @brief Add a dictionary over @c data. @c keepalive owns the bytes and is retained so
    /// they outlive the dictionary (e.g. a shared_ptr to a memory-mapped file). The
    /// dictionary is keyed by its Kind(). Throws if @c data is not a valid dictionary.
    void Add(RawData data, std::shared_ptr<void> keepalive);

    /// @brief Filter by selector, dispatching on kind. Mirrors the in-memory DictionarySet:
    /// a selector with no language defaults to "en"; an unknown kind or language yields an
    /// empty result (rather than throwing).
    [[nodiscard]] auto Filter(const Selector& selector, const TagSet& enabled_opt_ins = {}) const
        -> FilteredDictionaryPtr;

    [[nodiscard]] auto Contains(std::string_view kind) const -> bool {
        return dictionaries_.find(std::string{kind}) != dictionaries_.end();
    }

    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return dictionaries_.size();
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return dictionaries_.empty();
    }

private:
    static constexpr std::string_view kDefaultLanguage = "en";
    // Language-agnostic dictionaries (e.g. domain, shell) store words under the empty
    // language code; a no-language selector resolves to it when "en" is absent.
    static constexpr std::string_view kAgnosticLanguage = "";

    std::map<std::string, BinaryDictionary> dictionaries_;
    std::vector<std::shared_ptr<void>> keepalives_;
};

}  // namespace slugkit::generator::binary
