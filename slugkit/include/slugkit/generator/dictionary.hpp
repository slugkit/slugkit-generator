#pragma once

#include <slugkit/generator/dictionary_types.hpp>
#include <slugkit/generator/mixed_case_index.hpp>
#include <slugkit/generator/pattern.hpp>
#include <slugkit/generator/types.hpp>

#include <slugkit/compat/fast_pimpl.hpp>

#include <cstdint>
#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

namespace slugkit::generator {

/// @brief A filtered dictionary is a dictionary that contains only the
/// words that match the selector.
/// @note The filtered dictionary is immutable.
/// @note The filtered dictionary is only valid as long as the original dictionary is alive.
class FilteredDictionary {
public:
    // TODO use a more efficient storage type, we don't need kind and language here
    using WordContainer = std::vector<Word>;
    using WordContainerPtr = std::shared_ptr<WordContainer>;
    using Iterator = WordContainer::const_iterator;
    using StorageType = std::vector<Iterator>;

public:
    FilteredDictionary(WordContainerPtr words, CaseType case_type, StorageType&& storage, std::size_t max_length);

    std::string operator[](std::size_t index) const;

    CaseType GetCase() const {
        return case_type_;
    }

    const Word& GetWord(std::size_t index) const {
        return *words_[index];
    }

    std::size_t size() const {
        return words_.size();
    }

    bool empty() const {
        return words_.empty();
    }

    std::size_t GetMaxLength() const {
        return max_length_;
    }

    /// @brief Mixed-case capacity: the number of distinct cased forms across all filtered words.
    /// Only meaningful when GetCase() == kMixed; otherwise the layout is empty and this is 0.
    std::uint64_t MixedCapacity() const {
        return mixed_index_.Capacity();
    }

    /// @brief Decompose a permuted sequence value into (word index, compact case index) for the
    /// mixed-case path. @see MixedCaseIndex.
    std::pair<std::size_t, std::uint64_t> DecomposeMixed(std::uint64_t value) const {
        return mixed_index_.Decompose(value);
    }

private:
    // we hold the pointer to the original dictionary to avoid copying the words
    // and iterators not to be invalidated
    WordContainerPtr dictionary_;
    CaseType case_type_;
    StorageType words_;
    std::size_t max_length_;
    // Built only for kMixed selectors; maps sequence values to (word, case mask) without collisions.
    MixedCaseIndex mixed_index_;
};

using FilteredDictionaryPtr = std::shared_ptr<FilteredDictionary>;
using FilteredDictionaryConstPtr = std::shared_ptr<const FilteredDictionary>;

/// @brief A dictionary is a collection of words of the same kind
/// that are used to generate human-readable IDs.
/// @note The dictionary is immutable.
class Dictionary {
    using WordContainer = FilteredDictionary::WordContainer;
    using WordContainerPtr = FilteredDictionary::WordContainerPtr;

public:
    Dictionary(std::string_view kind, LanguageCodeView language, std::vector<Word> words, bool use_cache = true);

    Dictionary(const Dictionary& other) noexcept;
    Dictionary(Dictionary&& other) noexcept;
    Dictionary& operator=(const Dictionary& other) noexcept;
    Dictionary& operator=(Dictionary&& other) noexcept;

    ~Dictionary();

    const std::string& GetKind() const;
    const LanguageCode& GetLanguage() const;
    const Word& GetWord(std::size_t index) const;

    const std::string& operator[](std::size_t index) const;

    std::size_t size() const;

    bool empty() const;

    /// @brief Filters the dictionary by the selector.
    /// Will return an empty dictionary if the selector is empty or if the selector's kind is not the same as the
    /// dictionary's kind.
    /// @param selector The selector to use for filtering.
    /// @param enabled_opt_ins Opt-in tags lifted for this request (the "honest opt-in" usage
    /// flag). Accepted for signature parity with the binary dictionary; the in-memory dictionary
    /// carries no opt-in tag metadata today, so it is currently a no-op here.
    /// @return The filtered dictionary.
    FilteredDictionaryConstPtr Filter(const Selector& selector, const TagSet& enabled_opt_ins = {}) const;
    FilteredDictionaryConstPtr Filter(const EmojiGen::TagsType& include_tags, const EmojiGen::TagsType& exclude_tags)
        const;

    DictionaryStats GetStats() const;
    std::vector<TagDefinition> GetTagDefinitions() const;

private:
    // Impl size differs by toolchain/stdlib. The standalone (non-userver) FastPimpl treats the
    // constant as an upper bound (static_assert Size >= sizeof(Impl)), so it must cover the
    // largest standalone stdlib: libstdc++ (Linux) Impl is 96 bytes, libc++ (macOS/mobile) fits
    // within that. The userver build uses strict equality and its own value.
#ifdef SLUGKIT_USE_USERVER
    static constexpr std::size_t kPimplSize = 216UL;
    static constexpr std::size_t kPimplAlign = 8UL;
#else
    static constexpr std::size_t kPimplSize = 96UL;
    static constexpr std::size_t kPimplAlign = 8UL;
#endif

    struct Impl;
    slugkit::compat::FastPimpl<Impl, kPimplSize, kPimplAlign> pimpl_;
};

/// @brief A set of dictionaries that can be used to generate human-readable IDs.
/// @note The dictionary set is immutable.
class DictionarySet {
public:
    DictionarySet(std::vector<Dictionary> dictionaries);

    FilteredDictionaryConstPtr Filter(const Selector& selector, const TagSet& enabled_opt_ins = {}) const;

    auto size() const -> std::size_t {
        return dictionaries_.size();
    }

    /// @brief Parses a dictionary set from a format string.
    ///
    /// @param data The data to parse from.
    /// @return The parsed dictionary set.
    template <typename Format>
    static auto Parse(const std::string& data) -> DictionarySet;

    /// @brief Parses a dictionary set from a format stream.
    ///
    /// @param stream The stream to parse from.
    /// @return The parsed dictionary set.
    template <typename Format>
    static auto Parse(std::istream& stream) -> DictionarySet;

    /// @brief Parses a dictionary set from a format object.
    ///
    /// @param format The format object to parse from.
    /// @return The parsed dictionary set.
    template <typename Value>
    static auto Parse(const Value& value) -> DictionarySet;

private:
    std::map<std::string, Dictionary> dictionaries_;
    // a set of language-agnostic dictionaries
    std::set<std::string> language_agnostic_kinds_;
    // TODO LRU cache for filtered dictionaries
};

}  // namespace slugkit::generator
